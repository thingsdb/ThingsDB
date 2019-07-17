/*
 * ti/ex.c
 */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ti/ex.h>
#include <string.h>
#include <util/logger.h>
#include <uv.h>

static ex_t ex__e;

ex_t * ex_use(void)
{
    /* should only be called on the main thread */
    assert (Logger.main_thread == uv_thread_self());
    ex__e.nr = 0;
    ex__e.n = 0;
    ex__e.msg[0] = '\0';
    ex__e.msg[EX_MAX_SZ] = '\0';
    return &ex__e;
}

void ex_init(ex_t * e)
{
    e->nr = 0;
    e->n = 0;
    e->msg[0] = '\0';
    e->msg[EX_MAX_SZ] = '\0';
}

void ex_set(ex_t * e, ex_enum errnr, const char * errmsg, ...)
{
    va_list args;
    int n;
    e->nr = errnr;
    va_start(args, errmsg);
    n = vsnprintf(e->msg, EX_MAX_SZ, errmsg, args);
    e->n = n < EX_MAX_SZ ? n : EX_MAX_SZ;
    assert (e->n >= 0);
    va_end(args);
}

void ex_setn(ex_t * e, ex_enum errnr, const char * errmsg, size_t n)
{
    e->nr = errnr;
    e->n = n < EX_MAX_SZ ? n : EX_MAX_SZ;
    memcpy(e->msg, errmsg, e->n);
    e->msg[e->n] = '\0';
}

const char * ex_str(ex_enum errnr)
{
    switch (errnr)
    {
    case EX_OVERFLOW:           return EX_OVERFLOW_X;
    case EX_ZERO_DIV:           return EX_ZERO_DIV_X;
    case EX_MAX_QUOTA:          return EX_MAX_QUOTA_X;
    case EX_AUTH_ERROR:         return EX_AUTH_ERROR_X;
    case EX_FORBIDDEN:          return EX_FORBIDDEN_X;
    case EX_INDEX_ERROR:        return EX_INDEX_ERROR_X;
    case EX_BAD_DATA:           return EX_BAD_DATA_X;
    case EX_SYNTAX_ERROR:       return EX_SYNTAX_ERROR_X;
    case EX_NODE_ERROR:         return EX_NODE_ERROR_X;
    case EX_ASSERT_ERROR:       return EX_ASSERT_ERROR_X;
    case EX_REQUEST_TIMEOUT:    return EX_REQUEST_TIMEOUT_X;
    case EX_REQUEST_CANCEL:     return EX_REQUEST_CANCEL_X;
    case EX_WRITE_UV:           return EX_WRITE_UV_X;
    case EX_MEMORY:             return EX_MEMORY_X;
    case EX_INTERNAL:           return EX_INTERNAL_X;
    case EX_SUCCESS:            return EX_SUCCESS_X;
    }
    assert (0);
    return "unknown error";
}

