#include <locale.h>
#include <stdio.h>
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

    /* set threadpool size to 8 (default=4) */
    putenv("UV_THREADPOOL_SIZE=8");

    /* set default timezone to UTC */
    putenv("TZ=:UTC");
    tzset();

    rql_t * rql = rql_create();

    /* check rql and parse arguments */
    if (!rql || (rc = rql_args_parse(rql->args, argc, argv))) goto do_exit;

    if (rql->args->version)
    {
        printf(
                "RQL Server %s\n"
                "Build date: %s\n"
                "Maintainer: %s\n"
                "Home-page: %s\n",
#ifndef DEBUG
                RQL_VERSION,
#else
                RQL_VERSION "-DEBUG-RELEASE",
#endif
                RQL_BUILD_DATE,
                RQL_MAINTAINER,
                RQL_HOME_PAGE);

        goto do_exit;
    }

    rql_setup_logger(rql);

do_exit:
    rql_destroy(rql);

    return rc;
}
