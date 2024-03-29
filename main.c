/*
 * main.c - ThingsDB
 *
 * author/maintainer : Jeroen van der Heijden <jeroen@cesbit.com>
 * home page         : https://thingsdb.io
 * copyright         : 2024, Jeroen van der Heijden
 *
 * This code will be release as open source but the exact license might be
 * changed to something else than MIT.
 */
#include <fcntl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ti.h>
#include <ti/archive.h>
#include <ti/args.h>
#include <ti/cfg.h>
#include <ti/evars.h>
#include <ti/store.h>
#include <ti/tz.h>
#include <ti/user.h>
#include <ti/version.h>
#include <time.h>
#include <util/fx.h>
#include <util/logger.h>
#include <util/osarch.h>
#include <curl/curl.h>


static void main__init_deploy(void)
{
    size_t n;
    char * node_name = ti.cfg->node_name;

    if (fx_file_exist(ti.fn))
        return;

    n = strlen(node_name);
    if (!n)
        return;

    if (node_name[n-1] == '0' && (n == 1 || node_name[n-2] == '-'))
    {
        /* this is the fist node and ThingsDB is not yet initialized */
        ti.args->init = 1;
    }
    else
    {
        char * secret = getenv("THINGSDB_NODE_SECRET");
        secret = secret ? secret : node_name;

        memset(ti.args->secret, 0, ARGPARSE_MAX_LEN_ARG);
        strncpy(ti.args->secret, secret, ARGPARSE_MAX_LEN_ARG-1);
    }
}

int main(int argc, char * argv[])
{
    int seed, fd, rc = EXIT_SUCCESS;

    /* set local to LC_ALL and C to force a period over comma for float */
    (void) setlocale(LC_ALL, "C");

    /* initialize random */
    seed = 0;
    fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1 || read(fd, &seed, sizeof(int)) == -1)
    {
        printf("error reading /dev/urandom\n");
        return -1;
    }
    (void) close(fd);
    srand(seed);

    /* initialize global curl */
    curl_global_init(CURL_GLOBAL_ALL);

    /* set thread-pool size to 4 (default=4) */
    putenv("UV_THREADPOOL_SIZE=4");

    /* initialize time-zone information */
    ti_tz_init();

    printf(
"   _____ _   _             ____  _____    \n"
"  |_   _| |_|_|___ ___ ___|    \\| __  |   \n"
"    | | |   | |   | . |_ -|  |  | __ -|   \n"
"    |_| |_|_|_|_|_|_  |___|____/|_____|   version: "TI_VERSION"\n"
"                  |___|                   \n"
"\n");

    rc = ti_create();
    if (rc)
        goto stop;

    ti_evars_arg_parse();

    /* parse arguments */
    rc = ti_args_parse(argc, argv);
    if (rc)
    {
        rc = rc > 0 ? 0 : rc;
        goto stop;
    }

    if (ti.args->version)
    {
        ti_version_print();
        goto stop;
    }

    if (ti_init_logger())
    {
        printf("error initializing logger\n");
        rc = -1;
        goto stop;
    }

    osarch_init();
    log_info("running on: %s", osarch_get());

    if (*ti.args->config)
    {
        rc = ti_cfg_parse(ti.args->config);
        if (rc)
            goto stop;
    }

    ti_evars_cfg_parse();

    rc = ti_cfg_ensure_storage_path();
    if (rc)
        goto stop;

    rc = ti_lock();
    if (rc)
        goto stop;

    rc = ti_init();
    if (rc)
        goto stop;

    if (ti.args->deploy)
        main__init_deploy();

    if (ti.args->init)
    {
        if (fx_file_exist(ti.fn))
        {
            if (!ti.args->force)
            {
                log_warning(
                        "directory `%s` is already initialized, use --force "
                        "if you want to remove existing data and initialize "
                        "a new data directory",
                        ti.cfg->storage_path);
                goto load;
            }

            if (!ti_ask_continue("all data on this node will be removed"))
                goto stop;
        }

        if ((rc = ti_build()))
        {
            printf("error: building new ThingsDB cluster has failed\n");
            goto stop;
        }

        printf(
            "Well done, you successfully initialized ThingsDB!!\n\n"
            "You can now connect by using the "
            "default user `%s` and password `%s`\n\n",
            ti_user_def_name,
            ti_user_def_pass);

        goto run;
    }

    if (*ti.args->secret)
    {
        if (fx_file_exist(ti.fn))
        {
            if (!ti.args->force)
            {
                log_warning(
                        "directory `%s` is already initialized, use --force "
                        "if you want to remove existing data and wait for "
                        "a new join request",
                        ti.cfg->storage_path);
                goto load;
            }

            if (!ti_ask_continue("all data on this node will be removed"))
                goto stop;

            if (ti_rebuild())
            {
                printf("error while rebuilding\n");
                goto stop;
            }
        }

        ti_print_connect_info();
        goto run;
    }

    if (ti.args->forget_nodes)
    {
        if (!ti_ask_continue("all nodes information will be lost"))
            goto stop;

        if ((rc = ti_build_node()))
            goto stop;

        if ((rc = ti_store_restore()))
        {
            printf("error loading ThingsDB\n");
            goto stop;
        }
        goto run;
    }

load:
    if (fx_file_exist(ti.fn))
    {
        if ((rc = ti_read()))
        {
            printf("error reading ThingsDB from `%s`\n", ti.fn);
            goto stop;
        }

        if (ti.args->rebuild)
        {
            if (!ti_ask_continue("all data on this node will be removed"))
                goto stop;

            if (ti.nodes->vec->n < 2)
            {
                printf( "At least 2 nodes are required for a rebuild. "
                        "You might want to use --init or --secret with "
                        "--force instead.\n");
                goto stop;
            }

            if (ti_rebuild())
            {
                printf("error while rebuilding\n");
                goto stop;
            }
        }
        else if ((rc = ti_store_restore()))
        {
            printf("error loading ThingsDB\n");
            goto stop;
        }
    }
    else
    {
        printf(
            "The first time you should either setup ThingsDB by using "
            "the --init argument or use a one-time-secret using the --secret "
            "argument and wait for an invite from another node.\n");
        goto stop;
    }

run:
    fflush(stdout);
    rc = ti_run();

stop:
    if (ti_unlock() || rc)
    {
        rc = EXIT_FAILURE;
    }

    ti_destroy();

    /* cleanup global curl */
    curl_global_cleanup();

    if (rc)
        log_error("exit with error code %d", rc);
    else
        log_info("bye");
    return rc;
}
