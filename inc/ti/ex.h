/*
 * ti/ex.h
 */
#ifndef TI_EX_H_
#define TI_EX_H_

#define EX_MEMORY_S \
    "memory allocation error in `%s` at %s:%d", __func__, __FILE__, __LINE__

#define EX_INTERNAL_S \
    "internal error in `%s` at %s:%d", __func__, __FILE__, __LINE__

#define EX_RETURN_S \
    "return"

#define EX_MAX_SZ 16383

#define EX_MIN_ERR -127
#define EX_MAX_BUILD_IN_ERR -50

#define EX_RETURN_X             "success return"
#define EX_SUCCESS_X            "success"
#define EX_INTERNAL_X           "internal error"
#define EX_MEMORY_X             "memory allocation error"
#define EX_WRITE_UV_X           "cannot write to socket"
#define EX_REQUEST_CANCEL_X     "request is cancelled"
#define EX_REQUEST_TIMEOUT_X    "request timed out"
#define EX_ASSERT_ERROR_X       "assertion statement has failed"
#define EX_NODE_ERROR_X         "node is temporary unable to handle the request"
#define EX_SYNTAX_ERROR_X       "syntax error in query"
#define EX_BAD_DATA_X           "unable to handle request due to invalid data"
#define EX_INDEX_ERROR_X        "requested resource not found"
#define EX_FORBIDDEN_X          "forbidden (access denied)"
#define EX_AUTH_ERROR_X         "authentication error"
#define EX_MAX_QUOTA_X          "max quota is reached"
#define EX_ZERO_DIV_X           "division or module by zero"
#define EX_OVERFLOW_X           "integer overflow"


typedef enum
{
    /* free user space errors -EX_MIN_ERR..-100 */

    /* reserved build-in errors -99..-EX_MAX_BUILD_IN_ERR */

    /* defined build-in errors */
    EX_OVERFLOW             =-59,
    EX_ZERO_DIV             =-58,
    EX_MAX_QUOTA            =-57,
    EX_AUTH_ERROR           =-56,
    EX_FORBIDDEN            =-55,
    EX_INDEX_ERROR          =-54,
    EX_BAD_DATA             =-53,
    EX_SYNTAX_ERROR         =-52,
    EX_NODE_ERROR           =-51,
    EX_ASSERT_ERROR         =-50,  /* EX_MAX_BUILD_IN_ERR */

    /* reserved internal errors -49..-1 (not catchable with try() function) */

    /* defined internal errors */
    EX_REQUEST_TIMEOUT      =-5,
    EX_REQUEST_CANCEL       =-4,
    EX_WRITE_UV             =-3,
    EX_MEMORY               =-2,
    EX_INTERNAL             =-1,

    /* not an error */
    EX_SUCCESS              =0,
    EX_RETURN               =1,     /* internal, set by the return function */

} ex_enum;

typedef struct ex_s ex_t;

ex_t * ex_use(void);
void ex_init(ex_t * e);
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

#endif /* TI_EX_H_ */
