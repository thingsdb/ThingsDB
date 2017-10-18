#include <locale.h>

#include <stdlib.h>
#include <time.h>
#include <rql/rql.h>
#include <rql/user.h>
#include <rql/version.h>
#include <util/fx.h>


int main(int argc, char * argv[])
{
    int rc = 0;

    /*
     * set local to LC_ALL
     * more info at: http://www.cprogramming.com/tutorial/unicode.html
     */
    setlocale(LC_ALL, "");

    /* initialize random */
    srand(time(NULL));

    /* set threadpool size to 4 (default=4) */
    putenv("UV_THREADPOOL_SIZE=4");

    /* set default timezone to UTC */
    putenv("TZ=:UTC");
    tzset();

    rql_t * rql = rql_create();

    /* check rql and parse arguments */
    if (!rql || (rc = rql_args_parse(rql->args, argc, argv))) goto stop;

    if (rql->args->version)
    {
        rql_version_print();
        goto stop;
    }

    rql_init_logger(rql);

    if ((rc = rql_cfg_parse(rql->cfg, rql->args->config))) goto stop;
    if ((rc = rql_lock(rql))) goto stop;
    if ((rc = rql_init_fn(rql))) goto stop;

    if (rql->args->init)
    {
        if (fx_file_exist(rql->fn))
        {
            printf("error: directory '%s' is already initialized\n",
                    rql->cfg->rql_path);
            rc = -1;
            goto stop;
        }
        if ((rc = rql_build(rql)))
        {
            printf("error: building new pool has failed\n");
            goto stop;
        }

        printf(
            "Well done! You successfully initialized a new rql pool.\n"
            "Using the default user '%s' you can connect and do stuff!\n"
            "..before I forget, the password is '%s'\n\n",
            rql_user_def_name,
            rql_user_def_pass);

        goto stop;
    }
    else
    {
        if (fx_file_exist(rql->fn))
        {
            if ((rc = rql_read(rql)))
            {
                printf("error reading rql pool from: '%s'\n", rql->fn);
                goto stop;
            }
        }
        else
        {
            printf(
                "Waiting for a request to join some pool of rql nodes...\n"
                "(if you want to create a new pool instead, press CTRL+C and "
                "use the --init argument)\n");
        }
    }

    rc = rql_run(rql);
stop:
    if (!rc && rql_unlock(rql))
    {
        rc = -1;
    }
    rql_destroy(rql);

    return rc;
}
