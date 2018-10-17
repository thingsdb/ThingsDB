#include <locale.h>
#include <stdlib.h>
#include <thingsdb.h>
#include <ti/store.h>
#include <ti/user.h>
#include <ti/version.h>
#include <ti/store.h>
#include <util/fx.h>


int main(int argc, char * argv[])
{
    thingsdb_t * thingsdb;
    int rc = EXIT_SUCCESS;

    /* set local to LC_ALL */
    (void) setlocale(LC_ALL, "");

    /* initialize random */
    srand(time(NULL));

    /* set thread-pool size to 4 (default=4) */
    putenv("UV_THREADPOOL_SIZE=4");

    /* set default time-zone to UTC */
    putenv("TZ=:UTC");
    tzset();

    rc = thingsdb_init();
    if (!rc)
        goto stop;
    thingsdb = thingsdb_get();

    /* check tin and parse arguments */
    if ((rc = ti_args_parse(thingsdb->args, argc, argv)))
        goto stop;

    if (thingsdb->args->version)
    {
        ti_version_print();
        goto stop;
    }

    things_init_logger();

    if ((rc = ti_cfg_parse(thingsdb->cfg, thingsdb->args->config)))
        goto stop;
    if ((rc = thingsdb_lock()))
        goto stop;
    if ((rc = thingsdb_init_fn()))
        goto stop;

    if (thingsdb->args->init)
    {
        if (fx_file_exist(thingsdb->fn))
        {
            printf("error: directory '%s' is already initialized\n",
                    thingsdb->cfg->ti_path);
            rc = -1;
            goto stop;
        }
        if ((rc = thingsdb_build()))
        {
            printf("error: building new pool has failed\n");
            goto stop;
        }

        printf(
            "Well done! You successfully initialized a new tin pool.\n\n"
            "You can now star TIN and connect by using the default user `%s`.\n"
            "..before I forget, the password is '%s'\n\n",
            ti_user_def_name,
            ti_user_def_pass);

        goto stop;
    }
    else if (strlen(thingsdb->args->secret))
    {
        printf(
            "Waiting for a request to join some pool of tin nodes...\n"
            "(if you want to create a new pool instead, press CTRL+C and "
            "use the --init argument)\n");
    }
    else if (fx_file_exist(thingsdb->fn))
    {
        if ((rc = thingsdb_read()))
        {
            printf("error reading tin pool from: '%s'\n", thingsdb->fn);
            goto stop;
        }

        if ((rc = thingsdb_restore()))
        {
            printf("error loading tin pool\n");
            goto stop;
        }
    }
    else
    {
        printf(
            "The first time you should either create a new pool using "
            "the --init argument or set a one-time-secret using the --secret "
            "argument and wait for a request from another node to join.\n");
        goto stop;
    }

    rc = thingsdb_run();
stop:
    if (thingsdb_unlock() || rc)
    {
        rc = EXIT_FAILURE;
    }
    thingsdb_close();

    return rc;
}
