/*
 * ex.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <util/ex.h>

static struct
{
    int errnr;
    char errmsg[17];
} ex_alloc = {-1, "allocation error"};

void ex_destroy(ex_t * e)
{
    if (!*e || *e == (ex_t) &ex_alloc) return;
    free(*e);
    *e = NULL;
}

int ex_set(ex_t * e, int errnr, const char * errmsg, ...)
{
    int rc, sz = 128;
    if (*e) return -1;

    *e = malloc(sizeof(struct ex_s) + sz);
    if (!*e)
    {
        *e = (ex_t) &ex_alloc;
        return -1;
    }

    (*e)->errnr = errnr;

    va_list args;
    va_start(args, errmsg);
    rc = vsnprintf((*e)->errmsg, sz, errmsg, args);

    if (rc >= sz)
    {
        ex_t tmp = (ex_t) realloc(*e, sizeof(struct ex_s) + rc);
        if (!tmp)
        {
            free(*e);
            *e = (ex_t) &ex_alloc;
            return -1;
        }
        *e = tmp;
    }
    va_end(args);

    return rc;
}


