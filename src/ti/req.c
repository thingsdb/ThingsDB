/*
 * req.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/req.h>
#include <ti/write.h>
#include <ti/proto.h>
#include <ti.h>

static void ti__req_timeout(uv_timer_t * handle);
static void ti__req_write_cb(ti_write_t * req, ex_enum status);

int ti_req_create(
        ti_stream_t * stream,
        ti_pkg_t * pkg_req,
        uint32_t timeout,
        ti_req_cb cb,
        void * data)
{
    ti_req_t * prev;
    ti_req_t * req = malloc(sizeof(ti_req_t));
    if (!req)
        goto fail0;

    req->timer = malloc(sizeof(uv_timer_t));
    if (!req->timer)
        goto fail1;

    req->timer->data = req;

    assert (timeout > 0);
    assert (pkg_req->id == 0);

    /* we use package id 0 for fire-and-forget writes */
    pkg_req->id = ++stream->next_pkg_id
            ? stream->next_pkg_id
            : ++stream->next_pkg_id;

    req->stream = ti_grab(stream);
    req->pkg_req = pkg_req;
    req->pkg_res = NULL;
    req->data = data;
    req->cb_ = cb;

    prev = omap_set(stream->reqmap, pkg_req->id, req);
    if (!prev)
        goto fail2;

    if (prev != req)
    {
        ti_req_cancel(prev);
    }

    if (uv_timer_init(ti()->loop, req->timer))
        goto fail3;

    if (uv_timer_start(req->timer, ti__req_timeout, timeout * 1000, 0))
        goto fail4;

    if (ti_write(stream, pkg_req, req, ti__req_write_cb))
        goto fail5;

    return 0;

fail5:
    uv_timer_stop(req->timer);
fail4:
    uv_close((uv_handle_t *) req->timer, (uv_close_cb) &free);
    req->timer = NULL;
fail3:
    (void *) omap_rm(stream->reqmap, pkg_req->id);
fail2:
    ti_stream_drop(stream);
    free(req->timer);
fail1:
    free(req);
fail0:
    return -1;
}

void ti_req_destroy(ti_req_t * req)
{
    assert (req->timer == NULL);
    ti_stream_drop(req->stream);
    free(req);
}

/*
 * Request cancel is only called when the req is removed from the
 * reqs map.
 */
void ti_req_cancel(ti_req_t * req)
{
    if (!uv_is_closing((uv_handle_t *) req->timer))
    {
        uv_timer_stop(req->timer);
        uv_close((uv_handle_t *) req->timer, (uv_close_cb) &free);
        req->timer = NULL;
    }
    req->cb_(req, EX_REQUEST_CANCEL);
}

void ti_req_result(ti_req_t * req)
{
    if (!uv_is_closing((uv_handle_t *) req->timer))
    {
        uv_timer_stop(req->timer);
        uv_close((uv_handle_t *) req->timer, (uv_close_cb) &free);
        req->timer = NULL;
    }
    req->cb_(req, EX_SUCCESS);
}

static void ti__req_timeout(uv_timer_t * handle)
{
    ti_req_t * req = handle->data;

    (void *) omap_rm(req->stream->reqmap, req->pkg_req->id);

    log_warning(
            "timeout received on `%s` request to `%s`",
            ti_proto_str(req->pkg_req->tp),
            ti_stream_name(req->stream));

    uv_close((uv_handle_t *) req->timer, (uv_close_cb) &free);
    req->timer = NULL;
    req->cb_(req, EX_REQUEST_TIMEOUT);
}

/*
 * Actually a callback on rq_write.
 */
static void ti__req_write_cb(ti_write_t * wreq, ex_enum status)
{
    if (status)
    {
        ti_req_t * req = wreq->data;
        (void *) omap_rm(req->stream->reqmap, req->pkg_req->id);
        uv_timer_stop(req->timer);
        uv_close((uv_handle_t *) req->timer, (uv_close_cb) &free);
        req->timer = NULL;
        req->cb_(req, status);
    }
    ti_write_destroy(wreq);
}


