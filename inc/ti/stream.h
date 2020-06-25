/*
 * ti/stream.h
 */
#ifndef TI_STREAM_H_
#define TI_STREAM_H_

#include <stdint.h>
#include <ti/node.t.h>
#include <ti/pkg.t.h>
#include <ti/rpkg.t.h>
#include <ti/stream.t.h>
#include <ti/user.t.h>
#include <uv.h>

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


