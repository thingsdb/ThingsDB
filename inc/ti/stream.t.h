/*
 * ti/stream.t.h
 */
#ifndef TI_STREAM_T_H_
#define TI_STREAM_T_H_

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
    TI_STREAM_WS_IN_CLIENT,     /* WebSocket connection */
} ti_stream_enum;

typedef struct ti_stream_s  ti_stream_t;
typedef union ti_stream_via_u ti_stream_via_t;
typedef union ti_stream_with_u ti_stream_with_t;

#include <stdint.h>
#include <ti/node.t.h>
#include <ti/pkg.t.h>
#include <ti/user.t.h>
#include <ti/ws.t.h>
#include <util/omap.h>
#include <util/vec.h>
#include <uv.h>

typedef void (*ti_stream_pkg_cb)(ti_stream_t * stream, ti_pkg_t * pkg);

union ti_stream_via_u
{
    ti_user_t * user;       /* with reference */
    ti_node_t * node;       /* with reference */
};

union ti_stream_with_u
{
    uv_stream_t * uvstream;
    ti_ws_t * ws;
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
    ti_stream_with_t with;
    omap_t * reqmap;        /* ti_req_t waiting for response */
    vec_t * listeners;      /* weak reference to
                                    - ti_watch_t on client connections,
                                    - ti_syncer_t on node connections
                             */
};

struct ti_stream_req_s
{
    ti_stream_t * sock;
    ti_pkg_t * pkg;
};


#endif /* TI_STREAM_T_H_ */


