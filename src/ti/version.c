/*
 * version.c
 */
#include <stdlib.h>
#include <stdio.h>
#include <ti/version.h>

int ti_version_cmp(const char * version_a, const char * version_b)
{
    long int a, b;
    int i = 3;

    char * str_a = (char *) version_a;
    char * str_b = (char *) version_b;

    while (i--)
    {
        a = strtol(str_a, &str_a, 10);
        b = strtol(str_b, &str_b, 10);

        if (a != b)
            return a - b;
        else if (!*str_a || !*str_b)
            return 0;

        str_a++;
        str_b++;
    }

    return 0;
}

void ti_version_print(void)
{
    printf(
            "ThingsDB Node %s\n"
            "Maintainer: %s\n"
            "Home-page: %s\n",
            TI_VERSION,
            TI_MAINTAINER,
            TI_HOME_PAGE);
}
