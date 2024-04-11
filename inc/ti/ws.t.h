/*
 * ti/ws.t.h
 */
#ifndef TI_WS_T_H_
#define TI_WS_T_H_

#include <libwebsockets.h>
#include <ti/stream.t.h>
#include <util/queue.h>

typedef struct ti_ws_s ti_ws_t;

struct ti_ws_s
{
    queue_t * queue;          /* ti_write_t */
    ti_stream_t * stream;
    struct lws * wsi;
    unsigned char * wbuf;
    size_t wbuf_sz;
};

#endif  /* TI_WS_T_H_ */
