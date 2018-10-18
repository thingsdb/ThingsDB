/*
 * req.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <assert.h>
#include <thingsdb.h>
#include <stdlib.h>
#include <ti/req.h>
#include <ti/write.h>
static void ti__req_timeout(uv_timer_t * handle);
static void ti__req_write_cb(ti_write_t * req, int status);

int ti_req(
        ti_node_t * node,
        ti_pkg_t * pkg,
        uint32_t timeout,
        void * data,
        ti_req_cb cb)
{
    assert (timeout); // timeout is not allowed to be 0 */

    ti_req_t * req = (ti_req_t *) malloc(sizeof(ti_req_t));
    if (!req) return -1;

    req->node = ti_node_grab(node);
    req->data = data;
    req->pkg_req = pkg;
    req->cb_ = cb;
    while (!++node->req_next_id);
    req->id = node->req_next_id;

    ti_req_t * prev = imap_set(node->reqs, req->id, req);
    if (!prev) goto failed;

    if (prev != req)
    {
        ti_req_cancel(prev);
    }

    if (uv_timer_init(thingsdb_get()->loop, &req->timer) ||
        uv_timer_start(&req->timer, ti__req_timeout, timeout, 0)
    ) goto failed;

    if (ti_write(node->sock, pkg, req, ti__req_write_cb)) goto cancel;

    return 0;

cancel:
    imap_pop(node->reqs, req->id);
    uv_timer_stop(&req->timer);
    uv_close((uv_handle_t *) &req->timer, NULL);

failed:
    ti_node_drop(node);
    node->req_next_id--;
    free(req);
    return -1;
}

void ti_req_destroy(ti_req_t * req)
{
    ti_node_drop(req->node);
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

    log_warning("timeout received on '%s' req to node: '%s'",
            ti_back_req_str(req->pkg_req->tp), req->node->addr);

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
