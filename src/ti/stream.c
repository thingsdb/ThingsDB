/*
 * stream.c
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <ti.h>
#include <ti/pipe.h>
#include <ti/req.h>
#include <ti/stream.h>
#include <ti/tcp.h>
#include <ti/watch.h>
#include <ti/write.h>
#include <util/logger.h>

static void stream__write_pkg_cb(ti_write_t * req, ex_enum status);
static void stream__write_rpkg_cb(ti_write_t * req, ex_enum status);
static void stream__close_cb(uv_handle_t * uvstream);

#define STREAM__UNRESOLVED "unresolved";

static size_t stream__client_connections;

ti_stream_t * ti_stream_create(ti_stream_enum tp, ti_stream_pkg_cb cb)
{
    ti_stream_t * stream = calloc(1, sizeof(ti_stream_t));
    if (!stream)
        return NULL;

    stream->uvstream = malloc(sizeof(uv_stream_t));
    if (!stream->uvstream)
    {
        free(stream);
        return NULL;
    }

    stream->ref = 1;
    stream->tp = tp;
    stream->pkg_cb = cb;
    stream->uvstream->data = stream;

    switch (tp)
    {
    case TI_STREAM_TCP_OUT_NODE:
    case TI_STREAM_TCP_IN_NODE:
        stream->reqmap = omap_create();
        if (!stream->reqmap)
            break;
        if (uv_tcp_init(ti.loop, (uv_tcp_t *) stream->uvstream))
            break;
        return stream;

    case TI_STREAM_TCP_IN_CLIENT:
        ++stream__client_connections;
        if (uv_tcp_init(ti.loop, (uv_tcp_t *) stream->uvstream))
            break;
        return stream;

    case TI_STREAM_PIPE_IN_CLIENT:
        ++stream__client_connections;
        if (uv_pipe_init(ti.loop, (uv_pipe_t *) stream->uvstream, 0))
            break;
        return stream;
    }

    omap_destroy(stream->reqmap, NULL);
    free(stream);
    return NULL;
}

void ti_stream_drop(ti_stream_t * stream)
{
    if (stream && !--stream->ref)
    {
        assert (stream->flags & TI_STREAM_FLAG_CLOSED);
        log_info("closing stream `%s`", ti_stream_name(stream));
        uv_close((uv_handle_t *) stream->uvstream, stream__close_cb);
    }
}

void ti_stream_close(ti_stream_t * stream)
{
    if (!stream || (stream->flags & TI_STREAM_FLAG_CLOSED))
        return;
    stream->flags |= TI_STREAM_FLAG_CLOSED;
    stream->n = 0; /* prevents quick looping allocation function */
    omap_destroy(stream->reqmap, (omap_destroy_cb) &ti_req_cancel);
    stream->reqmap = NULL;
    ti_stream_drop(stream);
}

void ti_stream_stop_listeners(ti_stream_t * stream)
{
    if (!stream || !stream->listeners)
        return;
    vec_destroy(stream->listeners, (vec_destroy_cb) ti_watch_drop);
    stream->listeners = NULL;
}

void ti_stream_set_node(ti_stream_t * stream, ti_node_t * node)
{
    assert (stream->tp == TI_STREAM_TCP_OUT_NODE ||
            stream->tp == TI_STREAM_TCP_IN_NODE);
    assert (node->stream == NULL);
    assert (stream->via.node == NULL);

    /* clear the stream name */
    free(stream->name_);
    stream->name_ = NULL;

    stream->via.node = node;
    node->stream = stream;

    ti_incref(node);
}

void ti_stream_set_user(ti_stream_t * stream, ti_user_t * user)
{
    assert (stream->tp == TI_STREAM_TCP_IN_CLIENT ||
            stream->tp == TI_STREAM_PIPE_IN_CLIENT);

    if (stream->via.user)
        ti_user_drop(stream->via.user);

    /* clear the stream name */
    free(stream->name_);
    stream->name_ = NULL;

    stream->via.user = ti_grab(user);
}

void ti_stream_alloc_buf(uv_handle_t * handle, size_t sugsz, uv_buf_t * buf)
{
    ti_stream_t * stream = handle->data;

    if (!stream->n && stream->sz != sugsz)
    {
        free(stream->buf);
        stream->buf = malloc(sugsz);
        if (stream->buf == NULL)
        {
            log_error(EX_MEMORY_S);
            buf->base = NULL;
            buf->len = 0;
            return;
        }
        stream->sz = sugsz;
        stream->n = 0;
    }

    buf->base = stream->buf + stream->n;
    buf->len = stream->sz - stream->n;
}

void ti_stream_on_data(uv_stream_t * uvstream, ssize_t n, const uv_buf_t * buf)
{
    ti_stream_t * stream = uvstream->data;
    ti_pkg_t * pkg;
    size_t total_sz;

    if (stream->flags & TI_STREAM_FLAG_CLOSED)
    {
        log_error(
                "received %zd bytes from `%s` but the stream is closed",
                n, ti_stream_name(stream));
        return;
    }

    if (n < 0)
    {
        if (n != UV_EOF)
            log_error(uv_strerror(n));
        ti_stream_close(stream);
        return;
    }

    stream->n += n;
    if (stream->n < sizeof(ti_pkg_t))
        return;

    pkg = (ti_pkg_t *) stream->buf;
    if (!ti_pkg_check(pkg))
    {
        log_error(
                "invalid package (type=%u invert=%u size=%u) from `%s`, "
                "closing connection",
                pkg->tp, pkg->ntp, pkg->n, ti_stream_name(stream));
        ti_stream_close(stream);
        return;
    }

    total_sz = sizeof(ti_pkg_t) + pkg->n;

    if (stream->n < total_sz)
    {
        if (stream->sz < total_sz)
        {
            char * tmp = realloc(stream->buf, total_sz);
            if (!tmp)
            {
                log_error(EX_MEMORY_S);
                ti_stream_close(stream);
                return;
            }
            stream->buf = tmp;
            stream->sz = total_sz;
        }
        return;
    }

    stream->pkg_cb(stream, pkg);

    stream->n -= total_sz;
    if (stream->n > 0)
    {
        /* move data and call ti_stream_on_data() again */
        memmove(stream->buf, stream->buf + total_sz, stream->n);
        ti_stream_on_data(uvstream, 0, buf);
    }
}

/*
 * Returns 0 on success
 *
 *  - `toaddr` should be able to store at least INET6_ADDRSTRLEN
 */
int ti_stream_tcp_address(ti_stream_t * stream, char * toaddr)
{
    assert (stream->tp == TI_STREAM_TCP_OUT_NODE ||
            stream->tp == TI_STREAM_TCP_IN_NODE ||
            stream->tp == TI_STREAM_TCP_IN_CLIENT);

    struct sockaddr_storage name;
    int len = sizeof(name);     /* len is used both for input and output */

    if (uv_tcp_getpeername(
            (uv_tcp_t *) stream->uvstream,
            (struct sockaddr *) &name,
            &len))
        return -1;

    switch (name.ss_family)
    {
    case AF_INET:
        return uv_inet_ntop(
                AF_INET,
                &((struct sockaddr_in *) &name)->sin_addr,
                toaddr,
                INET6_ADDRSTRLEN);
    case AF_INET6:
        return uv_inet_ntop(
                AF_INET6,
                &((struct sockaddr_in6 *) &name)->sin6_addr,
                toaddr,
                INET6_ADDRSTRLEN);
    }

    assert (0);
    return -1;
}

const char * ti_stream_name(ti_stream_t * stream)
{
    char prefix[30];
    if (!stream)
        return "disconnected";

    if (stream->name_)
        return stream->name_;

    switch (stream->tp)
    {
    case TI_STREAM_TCP_OUT_NODE:
        if (stream->via.node)
            sprintf(prefix, "<node-out:%u> ", stream->via.node->id);
        else
            sprintf(prefix, "<node-out:not authorized> ");
        stream->name_ = ti_tcp_name(prefix, (uv_tcp_t *) stream->uvstream);
        return stream->name_ ? stream->name_ : "<node-out> "STREAM__UNRESOLVED;
    case TI_STREAM_TCP_IN_NODE:
        if (stream->via.node)
            sprintf(prefix, "<node-in:%u> ", stream->via.node->id);
        else
            sprintf(prefix, "<node-in:not authorized> ");
        stream->name_ = ti_tcp_name(prefix, (uv_tcp_t *) stream->uvstream);
        return stream->name_ ? stream->name_ : "<node-in> "STREAM__UNRESOLVED;
    case TI_STREAM_TCP_IN_CLIENT:
        if (stream->via.user)
            sprintf(prefix, "<client:%"PRIu64"> ", stream->via.user->id);
        else
            sprintf(prefix, "<client:not authorized> ");
        stream->name_ = ti_tcp_name(prefix, (uv_tcp_t *) stream->uvstream);
        return stream->name_ ? stream->name_ : "<client> "STREAM__UNRESOLVED;
    case TI_STREAM_PIPE_IN_CLIENT:
        if (stream->via.user)
            sprintf(prefix, "<client:%"PRIu64"> ", stream->via.user->id);
        else
            sprintf(prefix, "<client:not authorized> ");
        stream->name_ = ti_pipe_name(prefix, (uv_pipe_t *) stream->uvstream);
        return stream->name_ ? stream->name_ : "<client> "STREAM__UNRESOLVED;
    }

    assert (0);
    return STREAM__UNRESOLVED;
}

void ti_stream_on_response(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ti_req_t * req = omap_rm(stream->reqmap, pkg->id);
    if (!req)
    {
        log_warning(
                "received a response from `%s` on package id %u "
                "but no corresponding request is found "
                "(most likely the request has timed out)",
                ti_stream_name(stream), pkg->id);
        return;
    }
    req->pkg_res = pkg;
    ti_req_result(req);
}

int ti_stream_write_pkg(ti_stream_t * stream, ti_pkg_t * pkg)
{
    return ti_write(stream, pkg, NULL, stream__write_pkg_cb);
}

/* increases with a new reference as long as required */
int ti_stream_write_rpkg(ti_stream_t * stream, ti_rpkg_t * rpkg)
{
    ti_incref(rpkg);  /* BUG #214, increase before ti_write() */

    if (ti_write(stream, rpkg->pkg, rpkg, stream__write_rpkg_cb) == 0)
        return 0;

    ti_decref(rpkg);  /* roll-back the reference count */
    return -1;
}

size_t ti_stream_client_connections(void)
{
    return stream__client_connections;
}

static void stream__write_pkg_cb(ti_write_t * req, ex_enum status)
{
    (void)(status);     /* errors are logged by ti__write_cb() */
    free(req->pkg);
    ti_write_destroy(req);
}

static void stream__write_rpkg_cb(ti_write_t * req, ex_enum status)
{
    (void)(status);     /* errors are logged by ti__write_cb() */
    ti_rpkg_drop(req->data);
    ti_write_destroy(req);
}

static void stream__close_cb(uv_handle_t * uvstream)
{
    ti_stream_t * stream = uvstream->data;
    assert (stream);
    assert (stream->flags & TI_STREAM_FLAG_CLOSED);
    assert (stream->reqmap == NULL);

    switch ((ti_stream_enum) stream->tp)
    {
    case TI_STREAM_TCP_OUT_NODE:
    case TI_STREAM_TCP_IN_NODE:
        if (!stream->via.node)
            break;

        assert (stream->via.node->stream == stream);

        stream->via.node->status = TI_NODE_STAT_OFFLINE;
        stream->via.node->stream = NULL;

        ti_node_drop(stream->via.node);

        break;
    case TI_STREAM_TCP_IN_CLIENT:
    case TI_STREAM_PIPE_IN_CLIENT:
        --stream__client_connections;
        ti_user_drop(stream->via.user);
        break;
    }
    ti_stream_stop_listeners(stream);
    free(stream->buf);
    free(stream->name_);
    free(uvstream);
    free(stream);
}
