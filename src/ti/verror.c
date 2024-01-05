/*
 * ti/verror.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/verror.h>
#include <util/strx.h>

#define VERROR__MAX_MSG_SZ 56
#define VERROR__CACHE_SZ ((-EX_MIN_ERR) + 1)

typedef struct verror__s
{
    uint32_t ref;
    uint8_t tp;
    int8_t code;
    uint16_t msg_n;
    char msg[VERROR__MAX_MSG_SZ];       /* null terminated  */
} verror__t;


static verror__t verror__cache[VERROR__CACHE_SZ] = {};

static const verror__t verror__success = {
        .ref=1,
        .tp=TI_VAL_ERROR,
        .code=EX_SUCCESS,
        .msg_n=strlen(EX_SUCCESS_X),
        .msg=EX_SUCCESS_X
};
static const verror__t verror__internal = {
        .ref=1,
        .tp=TI_VAL_ERROR,
        .code=EX_INTERNAL,
        .msg_n=strlen(EX_INTERNAL_X),
        .msg=EX_INTERNAL_X
};
static const verror__t verror__memory = {
        .ref=1,
        .tp=TI_VAL_ERROR,
        .code=EX_MEMORY,
        .msg_n=strlen(EX_MEMORY_X),
        .msg=EX_MEMORY_X
};
static const verror__t verror__write_uv = {
        .ref=1,
        .tp=TI_VAL_ERROR,
        .code=EX_WRITE_UV,
        .msg_n=strlen(EX_WRITE_UV_X),
        .msg=EX_WRITE_UV_X
};
static const verror__t verror__request_cancel = {
        .ref=1,
        .tp=TI_VAL_ERROR,
        .code=EX_REQUEST_CANCEL,
        .msg_n=strlen(EX_REQUEST_CANCEL_X),
        .msg=EX_REQUEST_CANCEL_X
};
static const verror__t verror__request_timeout = {
        .ref=1,
        .tp=TI_VAL_ERROR,
        .code=EX_REQUEST_TIMEOUT,
        .msg_n=strlen(EX_REQUEST_TIMEOUT_X),
        .msg=EX_REQUEST_TIMEOUT_X
};
static const verror__t verror__assert_err = {
        .ref=1,
        .tp=TI_VAL_ERROR,
        .code=EX_ASSERT_ERROR,
        .msg_n=strlen(EX_ASSERT_ERROR_X),
        .msg=EX_ASSERT_ERROR_X
};
static const verror__t verror__node_err = {
        .ref=1,
        .tp=TI_VAL_ERROR,
        .code=EX_NODE_ERROR,
        .msg_n=strlen(EX_NODE_ERROR_X),
        .msg=EX_NODE_ERROR_X
};
static const verror__t verror__syntax_err = {
        .ref=1,
        .tp=TI_VAL_ERROR,
        .code=EX_SYNTAX_ERROR,
        .msg_n=strlen(EX_SYNTAX_ERROR_X),
        .msg=EX_SYNTAX_ERROR_X
};
static const verror__t verror__bad_data_err = {
        .ref=1,
        .tp=TI_VAL_ERROR,
        .code=EX_BAD_DATA,
        .msg_n=strlen(EX_BAD_DATA_X),
        .msg=EX_BAD_DATA_X
};
static const verror__t verror__lookup_err = {
        .ref=1,
        .tp=TI_VAL_ERROR,
        .code=EX_LOOKUP_ERROR,
        .msg_n=strlen(EX_LOOKUP_ERROR_X),
        .msg=EX_LOOKUP_ERROR_X
};
static const verror__t verror__forbidden_err = {
        .ref=1,
        .tp=TI_VAL_ERROR,
        .code=EX_FORBIDDEN,
        .msg_n=strlen(EX_FORBIDDEN_X),
        .msg=EX_FORBIDDEN_X
};
static const verror__t verror__auth_err = {
        .ref=1,
        .tp=TI_VAL_ERROR,
        .code=EX_AUTH_ERROR,
        .msg_n=strlen(EX_AUTH_ERROR_X),
        .msg=EX_AUTH_ERROR_X
};
static const verror__t verror__max_quota_err = {
        .ref=1,
        .tp=TI_VAL_ERROR,
        .code=EX_MAX_QUOTA,
        .msg_n=strlen(EX_MAX_QUOTA_X),
        .msg=EX_MAX_QUOTA_X
};
static const verror__t verror__zero_div_err = {
        .ref=1,
        .tp=TI_VAL_ERROR,
        .code=EX_ZERO_DIV,
        .msg_n=strlen(EX_ZERO_DIV_X),
        .msg=EX_ZERO_DIV_X
};
static const verror__t verror__overflow = {
        .ref=1,
        .tp=TI_VAL_ERROR,
        .code=EX_OVERFLOW,
        .msg_n=strlen(EX_OVERFLOW_X),
        .msg=EX_OVERFLOW_X
};
static const verror__t verror__value_err = {
        .ref=1,
        .tp=TI_VAL_ERROR,
        .code=EX_VALUE_ERROR,
        .msg_n=strlen(EX_VALUE_ERROR_X),
        .msg=EX_VALUE_ERROR_X
};
static const verror__t verror__type_err = {
        .ref=1,
        .tp=TI_VAL_ERROR,
        .code=EX_TYPE_ERROR,
        .msg_n=strlen(EX_TYPE_ERROR_X),
        .msg=EX_TYPE_ERROR_X
};
static const verror__t verror__num_arguments_err = {
        .ref=1,
        .tp=TI_VAL_ERROR,
        .code=EX_NUM_ARGUMENTS,
        .msg_n=strlen(EX_NUM_ARGUMENTS_X),
        .msg=EX_NUM_ARGUMENTS_X
};
static const verror__t verror__operation_err = {
        .ref=1,
        .tp=TI_VAL_ERROR,
        .code=EX_OPERATION,
        .msg_n=strlen(EX_OPERATION_X),
        .msg=EX_OPERATION_X
};
static const verror__t verror__cancelled_err = {
        .ref=1,
        .tp=TI_VAL_ERROR,
        .code=EX_CANCELLED,
        .msg_n=strlen(EX_CANCELLED_X),
        .msg=EX_CANCELLED_X
};

void ti_verror_init(void)
{
    /* internal */
    verror__cache[-EX_SUCCESS]          = verror__success;
    verror__cache[-EX_INTERNAL]         = verror__internal;
    verror__cache[-EX_MEMORY]           = verror__memory;

    /* other */
    verror__cache[-EX_WRITE_UV]         = verror__write_uv;
    verror__cache[-EX_REQUEST_CANCEL]   = verror__request_cancel;
    verror__cache[-EX_REQUEST_TIMEOUT]  = verror__request_timeout;
    verror__cache[-EX_ASSERT_ERROR]     = verror__assert_err;
    verror__cache[-EX_NODE_ERROR]       = verror__node_err;
    verror__cache[-EX_SYNTAX_ERROR]     = verror__syntax_err;
    verror__cache[-EX_BAD_DATA]         = verror__bad_data_err;
    verror__cache[-EX_LOOKUP_ERROR]     = verror__lookup_err;
    verror__cache[-EX_FORBIDDEN]        = verror__forbidden_err;
    verror__cache[-EX_AUTH_ERROR]       = verror__auth_err;
    verror__cache[-EX_MAX_QUOTA]        = verror__max_quota_err;
    verror__cache[-EX_ZERO_DIV]         = verror__zero_div_err;
    verror__cache[-EX_OVERFLOW]         = verror__overflow;
    verror__cache[-EX_VALUE_ERROR]      = verror__value_err;
    verror__cache[-EX_TYPE_ERROR]       = verror__type_err;
    verror__cache[-EX_NUM_ARGUMENTS]    = verror__num_arguments_err;
    verror__cache[-EX_OPERATION]        = verror__operation_err;
    verror__cache[-EX_CANCELLED]        = verror__cancelled_err;

    for (int i = 0; i < VERROR__CACHE_SZ; ++i)
    {
        if (verror__cache[i].ref == 0)
        {
            int sz;
            verror__cache[i].ref = 1;
            verror__cache[i].tp = TI_VAL_ERROR;
            verror__cache[i].code = -i;
            sz = sprintf(verror__cache[i].msg, "error:%d", -i);
            assert(sz > 0 && sz < VERROR__MAX_MSG_SZ);
            verror__cache[i].msg_n = (uint16_t) sz;
        }
    }
}

ti_verror_t * ti_verror_create(const char * msg, size_t n, int8_t code)
{
    ti_verror_t * verror = malloc(sizeof(ti_verror_t) + n + 1);
    if (!verror)
        return NULL;
    assert(n <= EX_MAX_SZ);
    verror->ref = 1;
    verror->tp = TI_VAL_ERROR;
    verror->code = code;
    verror->msg_n = n;
    memcpy(verror->msg, msg, n);
    verror->msg[n] = '\0';
    return verror;
}

ti_verror_t * ti_verror_from_code(int8_t code)
{
    ti_verror_t * verror;
    assert(code >= -127 && code <= 0);
    verror = (ti_verror_t *) &verror__cache[-code];
    ti_incref(verror);
    return verror;
}

void ti_verror_to_e(ti_verror_t * verror, ex_t * e)
{
    e->n = verror->msg_n;
    e->nr = verror->code;
    memcpy(e->msg, verror->msg, e->n);
    e->msg[e->n] = '\0';
}

/* return 0 if valid, < 0 if not */
int ti_verror_check_msg(const char * msg, size_t n, ex_t * e)
{
    assert(e->nr == 0);
    if (n > EX_MAX_SZ)
        ex_set(e, EX_VALUE_ERROR,
                "error messages should not exceed %d characters, "
                "got %zu characters", EX_MAX_SZ, n);
    else if (!strx_is_utf8n(msg, n))
        ex_set(e, EX_VALUE_ERROR,
                "error messages must have valid UTF8 encoding");
    return e->nr;
}
