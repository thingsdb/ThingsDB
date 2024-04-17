/*
 * ti/api.h
 */
#ifndef TI_API_H_
#define TI_API_H_

#include <ex.h>
#include <ti/api.t.h>
#include <ti/req.t.h>
#include <uv.h>

int ti_api_init(void);
_Bool ti_api_is_fwd_req(ti_req_t * req);
ti_api_request_t * ti_api_acquire(ti_api_request_t * api_request);
void ti_api_release(ti_api_request_t * api_request);
int ti_api_close_with_response(ti_api_request_t * ar, void * data, size_t size);
int ti_api_close_with_err(ti_api_request_t * api_request, ex_t * e);
void ti_api_close(ti_api_request_t * api_request);

static inline _Bool ti_api_is_closed(ti_api_request_t * ar)
{
    return ar->flags & TI_API_FLAG_IS_CLOSED;
}

static inline _Bool ti_api_check(void * data)
{
    return ((ti_api_request_t *) data)->id == TI_API_IDENTIFIER;
}

static inline _Bool ti_api_is_handle(uv_handle_t * handle)
{
    return
        handle->type == UV_TCP &&
        handle->data &&
        ((ti_api_request_t *) handle->data)->id == TI_API_IDENTIFIER;
}

#endif /* TI_API_H_ */
