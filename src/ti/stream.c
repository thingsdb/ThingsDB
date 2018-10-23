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
#include <ti.h>

static void ti__stream_stop(uv_handle_t * uvstream);
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
    stream->via.user = NULL;
    stream->pkg_cb = cb;
    stream->buf = NULL;
    stream->name_ = NULL;
    stream->reqmap = imap_create();
    stream->uvstream.data = stream;
    stream->next_pkg_id = 0;
    if (!stream->reqmap)
    {
        ti_stream_drop(stream);
    }

    return stream;
}

void ti_stream_drop(ti_stream_t * stream)
{
    assert(stream);
    if (stream && !--stream->ref)
    {
        assert (stream->reqmap == NULL);
        free(stream->buf);
        free(stream->name_);
        free(stream);
    }
}

void ti_stream_close(ti_stream_t * stream)
{
    stream->flags |= TI_STREAM_FLAG_CLOSED;
    stream->n = 0; /* prevents quick looping allocation function */
    imap_destroy(stream->reqmap, ti_req_cancel);
    stream->reqmap = NULL;
    uv_close((uv_handle_t *) &stream->uvstream, ti__stream_stop);
}

void ti_stream_alloc_buf(uv_handle_t * handle, size_t sugsz, uv_buf_t * buf)
{
    ti_stream_t * stream  = (ti_stream_t *) handle->data;

    if (!stream->n && stream->sz != sugsz)
    {
        free(stream->buf);
        stream->buf = (char *) malloc(sugsz);
        if (stream->buf == NULL)
        {
            log_error(EX_ALLOC);
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
                "invalid package from '%s'; closing connection",
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
                log_error(EX_ALLOC);
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
    if (stream->name_)
        return stream->name_;

    switch (stream->tp)
    {
    case TI_STREAM_TCP_OUT_NODE:
    case TI_STREAM_TCP_IN_NODE:
    case TI_STREAM_TCP_IN_CLIENT:
        stream->name_ = ti_tcp_name((uv_tcp_t *) &stream->uvstream);
        break;
    case TI_STREAM_PIPE_IN_CLIENT:
        stream->name_ = ti_pipe_name((uv_pipe_t *) &stream->uvstream);
        break;
    }

    return stream->name_ ? stream->name_ : ti__stream_name_unresolved;
}

static void ti__stream_stop(uv_handle_t * uvstream)
{
    ti_stream_t * stream = uvstream->data;
    switch ((ti_stream_enum) stream->tp)
    {
    case TI_STREAM_TCP_OUT_NODE:
    case TI_STREAM_TCP_IN_NODE:
        log_info("node disconnected: %s", ti_stream_name(stream));
        ti_node_drop(stream->via.node);
        break;
    case TI_STREAM_TCP_IN_CLIENT:
    case TI_STREAM_PIPE_IN_CLIENT:
        log_info("client disconnected: %s", ti_stream_name(stream));
        ti_user_drop(stream->via.user);
        break;
    }
    ti_stream_drop(stream);
}



