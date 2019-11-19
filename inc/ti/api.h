/*
 * ti/api.h
 */
#ifndef TI_API_H_
#define TI_API_H_

#include <lib/http_parser.h>
#include <uv.h>

#define TI_API_IDENTIFIER UINT32_MAX-1;

typedef struct ti_api_request_s ti_api_request_t;

int ti_api_init(void);
void ti_api_close(ti_api_request_t * api_request);
static inline _Bool ti_api_is_handle(uv_handle_t * handle);


struct ti_api_request_s
{
    uint32_t _id;               /* set to TI_API_IDENTIFIER */
    _Bool is_closed;
    uv_write_t req;
    uv_stream_t uvstream;
    http_parser parser;
    uv_buf_t * response;
};

static inline _Bool ti_api_is_handle(uv_handle_t * handle)
{
    return
        handle->type == UV_TCP &&
        handle->data &&
        ((ti_api_request_t *) handle->data)->_id == TI_API_IDENTIFIER;
}

#endif /* TI_API_H_ */
