/*
 * ti/ex.c
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ti/ex.h>

static ex_t ex__e;

ex_t * ex_use(void)
{
    ex__e.nr = 0;
    ex__e.n = 0;
    ex__e.msg[0] = '\0';
    return &ex__e;
}

void ex_set(ex_t * e, ex_e errnr, const char * errmsg, ...)
{
    e->nr = errnr;
    va_list args;
    va_start(args, errmsg);
    e->n = vsnprintf(e->msg, EX_MAX_SZ, errmsg, args);
    va_end(args);
}


