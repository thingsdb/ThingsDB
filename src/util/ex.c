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


ex_t * ex_new(size_t sz)
{
    ex_t * e = (ex_t *) malloc(sizeof(ex_t) + sz);
    if (!e) return NULL;
    e->sz = sz;
    e->n = 0;
    e->errnr = 0;
    return e;
}

int ex_set(ex_t * e, int errnr, const char * errmsg, ...)
{
    if (!e) return 0;
    int rc;
    e->errnr = errnr;
    va_list args;
    va_start(args, errmsg);
    rc = vsnprintf(e->errmsg, e->sz, errmsg, args);
    va_end(args);
    e->n = (size_t) rc;
    return rc;
}

