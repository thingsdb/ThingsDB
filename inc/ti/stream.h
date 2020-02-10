/*
 * ti/stream.h
 */
#ifndef TI_STREAM_H_
#define TI_STREAM_H_

enum
{
    TI_STREAM_FLAG_CLOSED           =1<<0,
    TI_STREAM_FLAG_SYNCHRONIZING    =1<<1,
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
#include <ti/rpkg.h>
#include <util/omap.h>
#include <util/vec.h>
typedef void (*ti_stream_pkg_cb)(ti_stream_t * stream, ti_pkg_t * pkg);

ti_stream_t * ti_stream_create(ti_stream_enum tp, ti_stream_pkg_cb cb);
void ti_stream_drop(ti_stream_t * sock);
void ti_stream_close(ti_stream_t * sock);
void ti_stream_stop_watching(ti_stream_t * stream);
void ti_stream_set_node(ti_stream_t * stream, ti_node_t * node);
void ti_stream_set_user(ti_stream_t * stream, ti_user_t * user);
void ti_stream_alloc_buf(uv_handle_t * handle, size_t sugsz, uv_buf_t * buf);
void ti_stream_on_data(uv_stream_t * uvstream, ssize_t n, const uv_buf_t * buf);
int ti_stream_tcp_address(ti_stream_t * stream, char * toaddr);
const char * ti_stream_name(ti_stream_t * stream);
void ti_stream_on_response(ti_stream_t * stream, ti_pkg_t * pkg);
int ti_stream_write_pkg(ti_stream_t * stream, ti_pkg_t * pkg);
int ti_stream_write_rpkg(ti_stream_t * stream, ti_rpkg_t * rpkg);
size_t ti_stream_client_connections(void);
static inline _Bool ti_stream_is_closed(ti_stream_t * stream);
static inline _Bool ti_stream_is_client(ti_stream_t * stream);

union ti_stream_u
{
    ti_user_t * user;       /* with reference */
    ti_node_t * node;       /* with reference */
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
    uv_stream_t * uvstream;
    omap_t * reqmap;        /* ti_req_t waiting for response */
    vec_t * watching;       /* weak reference to
                                    - ti_watch_t on client connections,
                                    - ti_syncer_t on node connections
                             */
};

struct ti_stream_req_s
{
    ti_stream_t * sock;
    ti_pkg_t * pkg;
};

static inline _Bool ti_stream_is_closed(ti_stream_t * stream)
{
    return !stream || (stream->flags & TI_STREAM_FLAG_CLOSED);
}

static inline _Bool ti_stream_is_client(ti_stream_t * stream)
{
    return stream && (
            stream->tp == TI_STREAM_TCP_IN_CLIENT ||
            stream->tp == TI_STREAM_PIPE_IN_CLIENT);
}

#endif /* TI_STREAM_H_ */


