/*
 * version.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <rql/version.h>
#include <stdlib.h>
#include <stdio.h>

int rql_version_cmp(const char * version_a, const char * version_b)
{
    long int a, b;

    char * str_a = (char *) version_a;
    char * str_b = (char *) version_b;

    while (1)
    {
        a = strtol(str_a, &str_a, 10);
        b = strtol(str_b, &str_b, 10);

        if (a != b) return a - b;
        if (!*str_a && !*str_b) return 0;
        if (!*str_a) return -1;
        if (!*str_b) return 1;

        str_a++;
        str_b++;
    }
}

void rql_version_print(void)
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
}
