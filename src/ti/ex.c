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
    case EX_OVERFLOW:
        return "integer overflow";
    case EX_ZERO_DIV:
        return "division or module by zero";
    case EX_MAX_QUOTA:
        return "max quota is reached";
    case EX_AUTH_ERROR:
        return "authentication error";
    case EX_FORBIDDEN:
        return "forbidden (access denied)";
    case EX_INDEX_ERROR:
        return "requested resource not found";
    case EX_BAD_DATA:
        return "unable to handle request due to invalid data";
    case EX_SYNTAX_ERROR:
        return "syntax error in query";
    case EX_NODE_ERROR:
        return "node is temporary unable to handle the request";
    case EX_ASSERT_ERROR:
        return "assertion statement has failed";
    case EX_REQUEST_TIMEOUT:
        return "request timed out";
    case EX_REQUEST_CANCEL:
        return "request is cancelled";
    case EX_WRITE_UV:
        return "cannot write to socket";
    case EX_MEMORY:
        return "memory allocation error";
    case EX_INTERNAL:
        return "internal error";
    case EX_SUCCESS:
        return "success";
    case EX_CUSTOM_01: case EX_CUSTOM_02: case EX_CUSTOM_03: case EX_CUSTOM_04:
    case EX_CUSTOM_05: case EX_CUSTOM_06: case EX_CUSTOM_07: case EX_CUSTOM_08:
    case EX_CUSTOM_09: case EX_CUSTOM_0A: case EX_CUSTOM_0B: case EX_CUSTOM_0C:
    case EX_CUSTOM_0D: case EX_CUSTOM_0E: case EX_CUSTOM_0F: case EX_CUSTOM_10:
    case EX_CUSTOM_11: case EX_CUSTOM_12: case EX_CUSTOM_13: case EX_CUSTOM_14:
    case EX_CUSTOM_15: case EX_CUSTOM_16: case EX_CUSTOM_17: case EX_CUSTOM_18:
    case EX_CUSTOM_19: case EX_CUSTOM_1A: case EX_CUSTOM_1B: case EX_CUSTOM_1C:
    case EX_CUSTOM_1D: case EX_CUSTOM_1E: case EX_CUSTOM_1F: case EX_CUSTOM_20:
        return "assertion statement has failed (with custom error code)";
    }
    assert (0);
    return "unknown";
}

