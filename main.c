#include <locale.h>

#include <stdlib.h>
#include <time.h>
#include <rql/rql.h>
#include <rql/version.h>


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
    if ((rc = rql_init_fn(rql))) goto stop;


stop:
    rql_destroy(rql);

    return rc;
}
