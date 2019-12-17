/*
 * main.c - ThingsDB
 *
 * author/maintainer : Jeroen van der Heijden <jeroen@transceptor.technology>
 * home page         : https://thingsdb.net
 * copyright         : 2019, Jeroen van der Heijden
 *
 * This code will be release as open source but the exact license might be
 * changed to something else than MIT.
 */
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/archive.h>
#include <ti/args.h>
#include <ti/cfg.h>
#include <ti/evars.h>
#include <ti/store.h>
#include <ti/user.h>
#include <ti/version.h>
#include <time.h>
#include <util/fx.h>
#include <util/logger.h>

int main(int argc, char * argv[])
{
    int rc = EXIT_SUCCESS;

    /* set local to LC_ALL and C to force a period over comma for float */
    (void) setlocale(LC_ALL, "C");

    /* initialize random */
    srand(time(NULL));

    /* set thread-pool size to 4 (default=4) */
    putenv("UV_THREADPOOL_SIZE=4");

    /* set default time-zone to UTC */
    putenv("TZ=:UTC");
    tzset();

    printf(
"   _____ _   _             ____  _____    \n"
"  |_   _| |_|_|___ ___ ___|    \\| __  |   \n"
"    | | |   | |   | . |_ -|  |  | __ -|   \n"
"    |_| |_|_|_|_|_|_  |___|____/|_____|   version: "TI_VERSION"\n"
"                  |___|                   \n"
"\n");
    fflush(stdout);

    rc = ti_create();
    if (rc)
        goto stop;

    /* parse arguments */
    rc = ti_args_parse(argc, argv);
    if (rc)
    {
        rc = rc > 0 ? 0 : rc;
        goto stop;
    }

    if (ti()->args->version)
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

    if (*ti()->args->config)
    {
        rc = ti_cfg_parse(ti()->args->config);
        if (rc)
            goto stop;
    }

    ti_evars_parse();

    rc = ti_cfg_ensure_storage_path();
    if (rc)
        goto stop;

    rc = ti_lock();
    if (rc)
        goto stop;

    rc = ti_init();
    if (rc)
        goto stop;

    if (ti()->args->init)
    {
        if (fx_file_exist(ti()->fn))
        {
            if (!ti()->args->force)
            {
                log_warning(
                        "directory `%s` is already initialized, use --force "
                        "if you want to remove existing data and initialize "
                        "a new data directory",
                        ti()->cfg->storage_path);
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

    if (*ti()->args->secret)
    {
        if (fx_file_exist(ti()->fn))
        {
            if (!ti()->args->force)
            {
                log_warning(
                        "directory `%s` is already initialized, use --force "
                        "if you want to remove existing data and wait for "
                        "a new join request",
                        ti()->cfg->storage_path);
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

    if (ti()->args->forget_nodes)
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
    if (fx_file_exist(ti()->fn))
    {
        if ((rc = ti_read()))
        {
            printf("error reading ThingsDB from `%s`\n", ti()->fn);
            goto stop;
        }

        if (ti()->args->rebuild)
        {
            if (!ti_ask_continue("all data on this node will be removed"))
                goto stop;

            if (ti()->nodes->imap->n < 2)
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
    rc = ti_run();

stop:
    if (ti_unlock() || rc)
    {
        rc = EXIT_FAILURE;
    }

    ti_destroy();
    if (rc)
        log_error("exit with error code %d", rc);
    else
        log_info("bye");
    logger_destroy();

    return rc;
}
