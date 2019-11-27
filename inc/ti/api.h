/*
 * ti/api.h
 */
#ifndef TI_API_H_
#define TI_API_H_

#include <lib/http_parser.h>
#include <uv.h>
#include <ti/user.h>
#include <ti/scope.h>
#include <ex.h>

#define TI_API_IDENTIFIER UINT32_MAX-1;

typedef enum
{
    TI_API_CT_UNKNOWN,
    TI_API_CT_JSON,
    TI_API_CT_MSGPACK,
} ti_api_content_t;

typedef enum
{
    TI_API_STATE_NONE,
    TI_API_STATE_CONTENT_TYPE,
    TI_API_STATE_AUTHORIZATION,
} ti_api_state_t;

typedef enum
{
    TI_API_FLAG_IS_CLOSED,
    TI_API_FLAG_IN_USE,
    TI_API_FLAG_INVALID_SCOPE,
} ti_api_flags_t;

typedef struct ti_api_request_s ti_api_request_t;

int ti_api_init(void);
ti_api_request_t * ti_api_acquire(ti_api_request_t * api_request);
void ti_api_release(ti_api_request_t * api_request);
int ti_api_close_with_err(ti_api_request_t * api_request);
void ti_api_close(ti_api_request_t * api_request);

struct ti_api_request_s
{
    uint32_t _id;               /* set to TI_API_IDENTIFIER */
    ti_api_flags_t flags;
    ti_api_content_t content_type;
    ti_api_state_t state;
    ti_scope_t scope;
    uv_write_t req;
    uv_stream_t uvstream;
    http_parser parser;
    ex_t e;
    size_t content_n;
    char * content;
    ti_user_t * user;
};

static inline _Bool ti_api_is_handle(uv_handle_t * handle)
{
    return
        handle->type == UV_TCP &&
        handle->data &&
        ((ti_api_request_t *) handle->data)->_id == TI_API_IDENTIFIER;
}

#endif /* TI_API_H_ */
