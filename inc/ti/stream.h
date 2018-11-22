/*
 * stream.h
 */
#ifndef TI_STREAM_H_
#define TI_STREAM_H_

enum
{
    TI_STREAM_FLAG_CLOSED   =1<<0,
};


typedef enum
{
    TI_STREAM_TCP_OUT_NODE,     /* TCP connection to other node */
    TI_STREAM_TCP_IN_NODE,      /* TCP connection from other node */
    TI_STREAM_TCP_IN_CLIENT,    /* TCP connection from client */
    TI_STREAM_PIPE_IN_CLIENT,   /* PIPE connection from client */
} ti_stream_enum;

typedef struct ti_stream_s  ti_stream_t;
typedef union ti_stream_u ti_stream_via_t;

#include <uv.h>
#include <stdint.h>
#include <ti/user.h>
#include <ti/node.h>
#include <ti/pkg.h>
#include <util/omap.h>
#include <util/vec.h>
typedef void (*ti_stream_pkg_cb)(ti_stream_t * stream, ti_pkg_t * pkg);

ti_stream_t * ti_stream_create(ti_stream_enum tp, ti_stream_pkg_cb cb);
void ti_stream_drop(ti_stream_t * sock);
int ti_stream_init(ti_stream_t * sock);
void ti_stream_close(ti_stream_t * sock);
void ti_stream_alloc_buf(uv_handle_t * handle, size_t sugsz, uv_buf_t * buf);
void ti_stream_on_data(uv_stream_t * uvstream, ssize_t n, const uv_buf_t * buf);
const char * ti_stream_name(ti_stream_t * stream);
void ti_stream_on_response(ti_stream_t * stream, ti_pkg_t * pkg);
static inline _Bool ti_stream_is_closed(ti_stream_t * stream);

union ti_stream_u
{
    ti_user_t * user;
    ti_node_t * node;
};

struct ti_stream_s
{
    uint32_t ref;
    uint32_t n;             /* buffer n */
    uint32_t sz;            /* buffer size */
    uint16_t next_pkg_id;
    uint8_t tp;
    uint8_t flags;
    ti_stream_via_t via;
    ti_stream_pkg_cb pkg_cb;
    char * buf;
    char * name_;
    uv_stream_t uvstream;
    omap_t * reqmap;        /* requests waiting for response */
    vec_t * watching;       /* weak reference to ti_watch_t */
};

struct ti_stream_req_s
{
    ti_stream_t * sock;
    ti_pkg_t * pkg;
};

static inline _Bool ti_stream_is_closed(ti_stream_t * stream)
{
    return stream->flags & TI_STREAM_FLAG_CLOSED;
}

#endif /* TI_STREAM_H_ */


