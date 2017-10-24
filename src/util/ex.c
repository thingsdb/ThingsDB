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

static ex_t ex__e;

ex_t * ex_use(void)
{
    ex__e.nr = 0;
    ex__e.msg[0] = '\0';
    return &ex__e;
}

int ex_set(ex_t * e, int errnr, const char * errmsg, ...)
{
    e->nr = errnr;
    int rc;
    va_list args;
    va_start(args, errmsg);
    rc = vsnprintf(e->msg, EX_MSG_SZ, errmsg, args);
    va_end(args);
    return rc;
}


