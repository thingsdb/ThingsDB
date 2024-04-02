/*
 * ti/ws.h
 */
#ifndef TI_WS_H_
#define TI_WS_H_

#include <ex.h>
#include <ti/ws.t.h>
#include <ti/req.t.h>
#include <uv.h>

int ti_ws_init(void);
void ti_ws_close(ti_ws_request_t * ws_request);


static inline _Bool ti_ws_is_handle(uv_handle_t * handle)
{
    return
        handle->type == UV_TCP &&
        handle->data &&
        ((ti_ws_request_t *) handle->data)->_id == TI_WS_IDENTIFIER;
}

#endif /* TI_WS_H_ */
