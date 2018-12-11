/*
 * stream.c
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <ti/stream.h>
#include <util/logger.h>
#include <ti/tcp.h>
#include <ti/pipe.h>
#include <ti/write.h>
#include <ti.h>

static void stream__write_pkg_cb(ti_write_t * req, ex_enum status);
static void stream__write_rpkg_cb(ti_write_t * req, ex_enum status);
static void stream__destroy(ti_stream_t * stream);
static void stream__close_cb(uv_handle_t * uvstream);

static const char * ti__stream_name_unresolved = "unresolved";


ti_stream_t * ti_stream_create(ti_stream_enum tp, ti_stream_pkg_cb cb)
{
    ti_stream_t * stream = malloc(sizeof(ti_stream_t));
    if (!stream)
        return NULL;

    stream->ref = 1;
    stream->n = 0;
    stream->sz = 0;
    stream->tp = tp;
    stream->flags = 0;
    stream->via.user = NULL;  /* set user/node to NULL */
    stream->pkg_cb = cb;
    stream->buf = NULL;
    stream->name_ = NULL;
    stream->reqmap = omap_create();
    stream->uvstream.data = stream;
    stream->next_pkg_id = 0;
    stream->watching = NULL;
    if (!stream->reqmap)
        goto failed;

    switch (tp)
    {
    case TI_STREAM_TCP_OUT_NODE:
    case TI_STREAM_TCP_IN_NODE:
    case TI_STREAM_TCP_IN_CLIENT:
        if (uv_tcp_init(ti()->loop, (uv_tcp_t *) &stream->uvstream))
            goto failed;
        break;
    case TI_STREAM_PIPE_IN_CLIENT:
        if (uv_pipe_init(ti()->loop, (uv_pipe_t *) &stream->uvstream, 0))
            goto failed;
        break;
    }

    return stream;

failed:
    omap_destroy(stream->reqmap, NULL);
    free(stream);
    return NULL;
}

void ti_stream_drop(ti_stream_t * stream)
{
    if (stream && !--stream->ref)
    {
        ti_stream_close(stream);

        omap_destroy(stream->reqmap, (omap_destroy_cb) &ti_req_cancel);
        stream->reqmap = NULL;

        log_info("closing stream `%s`", ti_stream_name(stream));

        uv_close((uv_handle_t *) &stream->uvstream, stream__close_cb);
    }
}

void ti_stream_close(ti_stream_t * stream)
{
    stream->flags |= TI_STREAM_FLAG_CLOSED;
    stream->n = 0; /* prevents quick looping allocation function */

    ti_stream_drop(stream);
}

void ti_stream_alloc_buf(uv_handle_t * handle, size_t sugsz, uv_buf_t * buf)
{
    ti_stream_t * stream  = (ti_stream_t *) handle->data;

    if (!stream->n && stream->sz != sugsz)
    {
        free(stream->buf);
        stream->buf = malloc(sugsz);
        if (stream->buf == NULL)
        {
            log_error(EX_ALLOC_S);
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
    ti_stream_t * stream  = uvstream->data;
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
                "invalid package from `%s`, closing connection",
                ti_stream_name(stream));
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
                log_error(EX_ALLOC_S);
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

const char * ti_stream_name(ti_stream_t * stream)
{
    if (!stream)
        return "disconnected";

    if (stream->name_)
        return stream->name_;

    switch (stream->tp)
    {
    case TI_STREAM_TCP_OUT_NODE:
        stream->name_ = ti_tcp_name(
                "<node-out> ",
                (uv_tcp_t *) &stream->uvstream);
        break;
    case TI_STREAM_TCP_IN_NODE:
        stream->name_ = ti_tcp_name(
                "<node-in> ",
                (uv_tcp_t *) &stream->uvstream);
        break;
    case TI_STREAM_TCP_IN_CLIENT:
        stream->name_ = ti_tcp_name(
                "<client> ",
                (uv_tcp_t *) &stream->uvstream);
        break;
    case TI_STREAM_PIPE_IN_CLIENT:
        stream->name_ = ti_pipe_name(
                "<client> ",
                (uv_pipe_t *) &stream->uvstream);
        break;
    }

    return stream->name_ ? stream->name_ : ti__stream_name_unresolved;
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

    ti_req_result(req);
}

int ti_stream_write_pkg(ti_stream_t * stream, ti_pkg_t * pkg)
{
    return ti_write(stream, pkg, NULL, stream__write_pkg_cb);
}

/* increases with a new reference as long as required */
int ti_stream_write_rpkg(ti_stream_t * stream, ti_rpkg_t * rpkg)
{
    if (ti_write(stream, rpkg->pkg, rpkg, stream__write_rpkg_cb))
        return -1;

    ti_incref(rpkg);
    return 0;
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

static void stream__destroy(ti_stream_t * stream)
{
    assert (stream->reqmap == NULL);

    switch ((ti_stream_enum) stream->tp)
    {
    case TI_STREAM_TCP_OUT_NODE:
    case TI_STREAM_TCP_IN_NODE:
        if (!stream->via.node)
            break;

        if (~stream->flags & TI_STREAM_FLAG_REPLACED)
        {
            assert (stream->via.node->stream == stream);
            stream->via.node->status = TI_NODE_STAT_OFFLINE;
            stream->via.node->stream = NULL;
        }
        else
        {
            assert (stream->via.node->stream != stream);
            log_debug("stream `%s` is replaced with `%s`",
                    ti_stream_name(stream),
                    ti_stream_name(stream->via.node->stream));
        }
        ti_node_drop(stream->via.node);
        break;
    case TI_STREAM_TCP_IN_CLIENT:
    case TI_STREAM_PIPE_IN_CLIENT:
        ti_user_drop(stream->via.user);
        break;
    }

    vec_destroy(stream->watching, (vec_destroy_cb) ti_watch_stop);
    free(stream->buf);
    free(stream->name_);
    free(stream);
}

static void stream__close_cb(uv_handle_t * uvstream)
{
    ti_stream_t * stream = uvstream->data;
    log_debug("disconnected `%s`", ti_stream_name(stream));
    stream__destroy(stream);
}


