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
static void ti__req_write_cb(ti_write_t * req, int status);

ti_req_t * ti_req(
        ti_stream_t * stream,
        ti_pkg_t * pkg,
        uint32_t timeout,
        ti_req_cb cb,
        void * data)
{
    ti_req_t * req = malloc(sizeof(ti_req_t));
    if (!req)
        return NULL;

    req->stream = ti_grab(stream);
    req->pkg_req = pkg;
    req->pkg_res = NULL;
    req->data = data;
    req->cb_ = cb;
    uv_timer_init(ti_get()->loop, &req->timer);


    ti_req_t * prev = imap_set(node->reqs, req->id, req);
    if (!prev) goto failed;

    if (prev != req)
    {
        ti_req_cancel(prev);
    }

    if (uv_timer_init(ti_get()->loop, &req->timer) ||
        uv_timer_start(&req->timer, ti__req_timeout, timeout, 0)
    ) goto failed;

    if (ti_write(node->stream, pkg, req, ti__req_write_cb)) goto cancel;

    return 0;

cancel:
    imap_pop(node->reqs, req->id);
    uv_timer_stop(&req->timer);


failed:
    ti_node_drop(node);
    node->req_next_id--;
    free(req);
    return -1;
}

void ti_req_destroy(ti_req_t * req)
{
    uv_close((uv_handle_t *) req->timer, free);
    ti_stream_drop(req->stream);
    free(req);
}

/*
 * Request cancel is only called when the req is removed from the
 * reqs map.
 */
void ti_req_cancel(ti_req_t * req)
{
    if (!uv_is_closing((uv_handle_t *) &req->timer))
    {
        uv_timer_stop(&req->timer);
        uv_close((uv_handle_t *) &req->timer, NULL);
    }
    req->cb_(req, EX_REQUEST_CANCEL);
}

static void ti__req_timeout(uv_timer_t * handle)
{
    ti_req_t * req = (ti_req_t *) handle->data;

    imap_pop(req->node->reqs, req->id);

    log_warning(
            "timeout received on '%s' req to node: '%s'",
            ti_proto_str(req->pkg_req->tp),
            req->node->addr);

    uv_close((uv_handle_t *) &req->timer, NULL);

    req->cb_(req, EX_REQUEST_TIMEOUT);
}

/*
 * Actually a callback on rq_write.
 */
static void ti__req_write_cb(ti_write_t * wreq, ex_e status)
{
    if (status)
    {
        ti_req_t * req = (ti_req_t *) wreq->data;

        imap_pop(req->node->reqs, req->id);

        uv_timer_stop(&req->timer);
        uv_close((uv_handle_t *) &req->timer, NULL);

        req->cb_(req, status);
    }
    ti_write_destroy(wreq);
}
