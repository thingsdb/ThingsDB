/*
 * ex.c
 */
#include <assert.h>
#include <ex.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <util/logger.h>
#include <uv.h>

void ex_set(ex_t * e, ex_enum errnr, const char * errmsg, ...)
{
    va_list args;
    int n;
    e->nr = errnr;
    va_start(args, errmsg);
    n = vsnprintf(e->msg, EX_MAX_SZ, errmsg, args);
    e->n = n < EX_MAX_SZ ? n : EX_MAX_SZ;

    /* TODO: Check below may be removed when we are sure no wrong
     *       formatting in the code exists */
    if (e->n < 0)
    {
        e->n = 0;
        e->msg[0] = '\0';
        assert(0);
    }

    va_end(args);
}

void ex_setn(ex_t * e, ex_enum errnr, const char * errmsg, size_t n)
{
    e->nr = errnr;
    e->n = n < EX_MAX_SZ ? n : EX_MAX_SZ;
    memcpy(e->msg, errmsg, e->n);
    e->msg[e->n] = '\0';
}

void ex_append(ex_t * e, const char * errmsg, ...)
{
    va_list args;
    int n;
    va_start(args, errmsg);
    n = e->n + vsnprintf(e->msg + e->n, EX_MAX_SZ - e->n, errmsg, args);
    e->n = n < EX_MAX_SZ ? n : EX_MAX_SZ;
    assert (e->n >= 0);
    va_end(args);
}

const char * ex_str(ex_enum errnr)
{
    switch (errnr)
    {
    /* build-in */
    case EX_CANCELLED:          return EX_CANCELLED_X;
    case EX_OPERATION:          return EX_OPERATION_X;
    case EX_NUM_ARGUMENTS:      return EX_NUM_ARGUMENTS_X;
    case EX_TYPE_ERROR:         return EX_TYPE_ERROR_X;
    case EX_VALUE_ERROR:        return EX_VALUE_ERROR_X;
    case EX_OVERFLOW:           return EX_OVERFLOW_X;
    case EX_ZERO_DIV:           return EX_ZERO_DIV_X;
    case EX_MAX_QUOTA:          return EX_MAX_QUOTA_X;
    case EX_AUTH_ERROR:         return EX_AUTH_ERROR_X;
    case EX_FORBIDDEN:          return EX_FORBIDDEN_X;
    case EX_LOOKUP_ERROR:       return EX_LOOKUP_ERROR_X;
    case EX_BAD_DATA:           return EX_BAD_DATA_X;
    case EX_SYNTAX_ERROR:       return EX_SYNTAX_ERROR_X;
    case EX_NODE_ERROR:         return EX_NODE_ERROR_X;
    case EX_ASSERT_ERROR:       return EX_ASSERT_ERROR_X;

    /* internal, non catchable */
    case EX_RESULT_TOO_LARGE:   return EX_RESULT_TOO_LARGE_X;
    case EX_REQUEST_TIMEOUT:    return EX_REQUEST_TIMEOUT_X;
    case EX_REQUEST_CANCEL:     return EX_REQUEST_CANCEL_X;
    case EX_WRITE_UV:           return EX_WRITE_UV_X;
    case EX_MEMORY:             return EX_MEMORY_X;
    case EX_INTERNAL:           return EX_INTERNAL_X;

    /* success */
    case EX_SUCCESS:            return EX_SUCCESS_X;
    case EX_RETURN:             return EX_RETURN_X;
    }
    /* probably a custom error */
    return "error";
}

