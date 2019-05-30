/*
 * ti/web.h
 */
#ifndef TI_WEB_H_
#define TI_WEB_H_

#include <lib/http_parser.h>
#include <uv.h>

#define TI_WEB_MAX_HTTP_HEADERS 20
#define TI_WEB_IDENTIFIER UINT64_MAX;

typedef struct ti_web_header_s ti_web_header_t;
typedef struct ti_web_request_s ti_web_request_t;


int ti_web_init(void);
void ti_web_close(ti_web_request_t * web_request);
static inline _Bool ti_web_is_handle(uv_handle_t * handle);

struct ti_web_header_s
{
    const char* field;
    const char* value;
    size_t field_length;
    size_t value_length;
};

struct ti_web_request_s
{
    uint64_t _id;               /* set to TI_WEB_IDENTIFIER */
    _Bool is_closed;
    uv_write_t req;
    uv_stream_t uvstream;
    http_parser parser;
    uv_buf_t * response;
};

static inline _Bool ti_web_is_handle(uv_handle_t * handle)
{
    return
        handle->type == UV_TCP &&
        handle->data &&
        ((ti_web_request_t *) handle->data)->_id == TI_WEB_IDENTIFIER;
}

#endif /* TI_WEB_H_ */
