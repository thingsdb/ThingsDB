/*
 * ex.h
 */
#ifndef EX_H_
#define EX_H_

#define EX_MEMORY_S \
    "memory allocation error in `%s` at %s:%d", __func__, __FILE__, __LINE__

#define EX_INTERNAL_S \
    "internal error in `%s` at %s:%d", __func__, __FILE__, __LINE__

#define EX_RETURN_S \
    "return"

#define EX_MAX_SZ 16383

#define EX_MIN_ERR -127
#define EX_MAX_BUILD_IN_ERR -50

/* success */
#define EX_RETURN_X             "success return"
#define EX_SUCCESS_X            "success"

/* internal, non catchable */
#define EX_INTERNAL_X           "internal error"
#define EX_MEMORY_X             "memory allocation error"
#define EX_WRITE_UV_X           "cannot write to socket"
#define EX_REQUEST_CANCEL_X     "request is cancelled"
#define EX_REQUEST_TIMEOUT_X    "request timed out"
#define EX_RESULT_TOO_LARGE_X   "result too large"

/* build-in */
#define EX_ASSERT_ERROR_X       "assertion statement has failed"
#define EX_NODE_ERROR_X         "node is temporary unable to handle the request"
#define EX_SYNTAX_ERROR_X       "syntax error in query"
#define EX_BAD_DATA_X           "unable to handle request due to invalid data"
#define EX_LOOKUP_ERROR_X       "requested resource not found"
#define EX_FORBIDDEN_X          "forbidden (access denied)"
#define EX_AUTH_ERROR_X         "authentication error"
#define EX_MAX_QUOTA_X          "max quota is reached"
#define EX_ZERO_DIV_X           "division or module by zero"
#define EX_OVERFLOW_X           "integer overflow"
#define EX_VALUE_ERROR_X        "object has the right type but an inappropriate value"
#define EX_TYPE_ERROR_X         "object of inappropriate type"
#define EX_NUM_ARGUMENTS_X      "wrong number of arguments"
#define EX_OPERATION_ERROR_X    "operation is not valid in the current context"

typedef enum
{
    /* free user space errors -EX_MIN_ERR..-100 */

    /* reserved build-in errors -99..-EX_MAX_BUILD_IN_ERR */

    /* build-in errors */
    EX_OPERATION_ERROR      =-63,
    EX_NUM_ARGUMENTS        =-62,
    EX_TYPE_ERROR           =-61,
    EX_VALUE_ERROR          =-60,
    EX_OVERFLOW             =-59,
    EX_ZERO_DIV             =-58,
    EX_MAX_QUOTA            =-57,
    EX_AUTH_ERROR           =-56,
    EX_FORBIDDEN            =-55,
    EX_LOOKUP_ERROR         =-54,
    EX_BAD_DATA             =-53,
    EX_SYNTAX_ERROR         =-52,
    EX_NODE_ERROR           =-51,
    EX_ASSERT_ERROR         =-50,  /* EX_MAX_BUILD_IN_ERR */

    /* reserved internal errors -49..-1 (not catchable with try() function) */

    /* internal errors */
    EX_RESULT_TOO_LARGE     =-6,
    EX_REQUEST_TIMEOUT      =-5,
    EX_REQUEST_CANCEL       =-4,
    EX_WRITE_UV             =-3,
    EX_MEMORY               =-2,
    EX_INTERNAL             =-1,

    /* success */
    EX_SUCCESS              =0,
    EX_RETURN               =1,     /* internal, set by the return function */

} ex_enum;

typedef struct ex_s ex_t;

#include <inttypes.h>
#include <stddef.h>
#include <string.h>

void ex_set(ex_t * e, ex_enum errnr, const char * errmsg, ...);
void ex_setn(ex_t * e, ex_enum errnr, const char * errmsg, size_t n);
void ex_append(ex_t * e, const char * errmsg, ...);
const char * ex_str(ex_enum errnr);
_Bool ex_int64_is_errnr(int64_t i);

struct ex_s
{
    ex_enum nr;
    int n;
    char msg[EX_MAX_SZ + 1];    /* 0 terminated message */
};

#define ex_set_mem(e__) ex_set((e__), EX_MEMORY, EX_MEMORY_S)
#define ex_set_internal(e__) ex_set((e__), EX_INTERNAL, EX_INTERNAL_S)
#define ex_set_return(e__) ex_setn((e__), EX_RETURN, EX_RETURN_S, 6)

static inline void ex_clear(ex_t * e)
{
    memset(e, 0, sizeof(ex_t));
}

#endif /* EX_H_ */
