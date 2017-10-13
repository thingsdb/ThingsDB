/*
 * req.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <rql/req.h>

static void rql__req_timeout(uv_timer_t * handle);
static void rql__req_write_cb(rql_write_t * req, int status);

int rql_req(
        rql_node_t * node,
        rql_pkg_t * pkg,
        uint32_t timeout,
        void * data,
        rql_req_cb cb)
{
    assert (timeout); // timeout is not allowed to be 0 */

    rql_req_t * req = (rql_req_t *) malloc(sizeof(rql_req_t));
    if (!req) return -1;

    req->node = rql_node_grab(node);
    req->data = data;
    req->pkg_req = pkg;
    while (!++node->req_next_id);
    req->id = node->req_next_id;

    rql_req_t * prev = imap_set(node->reqs, req->id, req);
    if (!prev) goto failed;

    if (prev != req)
    {
        rql_req_cancel(prev);
    }

    if (uv_timer_init(node->sock->rql->loop, &req->timer) ||
        uv_timer_start(&req->timer, rql__req_timeout, timeout, 0)
    ) goto failed;

    if (rql_write(node->sock, pkg, req, rql__req_write_cb)) goto cancel;

    return 0;

cancel:
    imap_pop(node->reqs, req->id);
    uv_timer_stop(req->timer);
    uv_close((uv_handle_t *) req->timer, NULL);

failed:
    rql_node_drop(node);
    node->req_next_id--;
    free(req);
    return -1;
}

void rql_req_destroy(rql_req_t * req)
{
    rql_node_drop(req->node);
    free(req);
}

/*
 * Request cancel is only called when the req is removed from the
 * reqs map.
 */
void rql_req_cancel(rql_req_t * req)
{
    if (!uv_is_closing(req->timer))
    {
        uv_timer_stop(req->timer);
        uv_close((uv_handle_t *) req->timer, NULL);
    }
    req->cb_(req, EX_REQUEST_CANCEL);
}

static void rql__req_timeout(uv_timer_t * handle)
{
    rql_req_t * req = (rql_req_t *) handle->data;

    imap_pop(req->node->reqs, req->id);

    log_warning("timeout received on '%s' req to node: '%s'",
            rql_back_req_str(req->pkg_req->tp), req->node->addr);

    uv_close((uv_handle_t *) req->timer, NULL);

    req->cb_(req, EX_REQUEST_TIMEOUT);
}

/*
 * Actually a callback on rq_write.
 */
static void rql__req_write_cb(rql_write_t * wreq, ex_e status)
{
    if (status)
    {
        rql_req_t * req = (rql_req_t *) wreq->data;

        imap_pop(req->node->reqs, req->id);

        uv_timer_stop(req->timer);
        uv_close((uv_handle_t *) req->timer, NULL);

        req->cb_(req, status);
    }
    rql_write_destroy(wreq);
}
