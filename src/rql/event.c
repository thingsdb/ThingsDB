/*
 * event.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <assert.h>
#include <stdlib.h>
#include <qpack.h>
#include <rql/event.h>
#include <rql/req.h>
#include <rql/access.h>
#include <rql/proto.h>
#include <rql/task.h>
#include <rql/rql.h>
#include <rql/nodes.h>
#include <util/link.h>
#include <util/queue.h>
#include <util/ex.h>
#include <util/qpx.h>

const int rql__event_reg_timeout = 10;
const int rql__event_upd_timeout = 10;
const int rql__event_ready_timeout = 30;
const int rql__event_cancel_timeout = 5;

static int rql__event_to_queue(rql_event_t * event);
static int rql__event_reg(rql_event_t * event);
static int rql__event_upd(rql_event_t * event, uint64_t prev_id);
static void rql__event_on_reg_cb(rql_prom_t * prom);
static int rql__event_ready(rql_event_t * event);
static void rql__event_on_ready_cb(rql_prom_t * prom);
static int rql__event_cancel(rql_event_t * event);
static void rql__event_on_cancel_cb(rql_prom_t * prom);
//inline static int rql__event_cmp(rql_event_t * a, rql_event_t * b);
static void rql__event_unpack(
        rql_event_t * event,
        qp_unpacker_t * unpacker,
        ex_t * e);

rql_event_t * rql_event_create(rql_events_t * events)
{
    rql_event_t * event = malloc(sizeof(rql_event_t));
    if (!event) return NULL;
    event->events = events;
    event->target = NULL;
    event->node = NULL;
    event->raw = NULL;
    event->client = NULL;
    event->refelems = NULL;
    event->tasks = vec_new(1);
    event->result = qpx_packer_create(16);
    event->status = RQL_EVENT_STAT_UNINITIALIZED;
    event->nodes = NULL;
    event->prom = NULL;

    if (!event->tasks)
    {
        rql_event_destroy(event);
        return NULL;
    }

    return event;
}

void rql_event_destroy(rql_event_t * event)
{
    if (!event) return;
    if (event->events)
    {
        /* the event might be in the queue */
        queue_remval(event->events->queue, event);
    }
    rql_db_drop(event->target);
    rql_node_drop(event->node);
    rql_sock_drop(event->client);
    vec_destroy(event->tasks, (vec_destroy_cb) rql_task_destroy);
    vec_destroy(event->nodes, (vec_destroy_cb) rql_node_drop);
    qpx_packer_destroy(event->result);
    free(event->prom);
    free(event->raw);
    free(event);
}

void rql_event_new(rql_sock_t * sock, rql_pkg_t * pkg, ex_t * e)
{
    rql_event_t * event = rql_event_create(sock->rql->events);
    if (!event || !(event->nodes = vec_new(event->events->rql->nodes->n - 1)))
    {
        ex_set_alloc(e);
        goto failed;
    }

    event->id = event->events->next_id;
    event->pid = pkg->id;
    event->node = rql_node_grab(event->events->rql->node);
    event->client = rql_sock_grab(sock);
    event->status = RQL_EVENT_STAT_REG;

    if (!rql_nodes_has_quorum(sock->rql->nodes))
    {
        ex_set(e, RQL_PROTO_NODE_ERR,
                "node '%s' does not have the required quorum",
                sock->rql->node->addr);
        goto failed;
    }

    rql_event_raw(event, pkg->data, pkg->n, e);
    if (e) goto failed;

    if (rql__event_to_queue(event) || rql__event_reg(event))
    {
        ex_set_alloc(e);
        goto failed;
    }

    return;  /* success */

failed:
    assert (e);
    rql_event_destroy(event);
}

int rql_event_init(rql_event_t * event)
{
    event->node = rql_node_grab(event->events->rql->node);
    event->id = event->events->next_id;
    event->status = RQL_EVENT_STAT_REG;
    event->nodes = vec_new(event->events->rql->nodes->n - 1);
    return -!event->nodes;
}

//int rql_event_fill(
//        rql_event_t * event,
//        uint64_t id,
//        rql_node_t * node,
//        rql_db_t * target)
//{
//    rql_t * rql = event->rql;
//    if (id > rql->event_max_id)
//    {
//        if (link_unshift(rql->queue, event))
//        {
//            log_error(EX_ALLOC);
//            return -1;
//        }
//        rql->event_max_id = id;;
//        goto accept;
//    }
//
//    if (id <= rql->event_cur_id)
//    {
//        log_warning("reject event id: %"PRIu64" (current id: %"PRIu64")",
//                id, rql->event_cur_id);
//        return -1;
//    }
//
//    for (link_each(event->rql->queue, rql_event_t, ev))
//    {
//        if (id ev->target != target) continue;
//        assert (ev->id > event->id)
//        if (ev->id < event->id) goto accept;
//        if (rql__event_cmp(event, ev) < 0) return -1;
//
//        assert (ev->status <= RQL_EVENT_STAT_WAIT_ACCEPT);
//
//        link_pop_current(event->rql->queue);
//        rql_event_destroy(ev);
//    }
//
//accept:
//    //    *rql__event_max_id(event) = event->id;
//    event->id = id;
//    event->target = (target) ? rql_db_grab(target) : NULL;
//    event->node = rql_node_grab(node);
//    return 0;
//}

void rql_event_raw(
        rql_event_t * event,
        const unsigned char * raw,
        size_t sz,
        ex_t * e)
{
    qp_obj_t target;
    qp_unpacker_t unpacker;
    qpx_unpacker_init(&unpacker, raw, sz);

    if (!qp_is_map(qp_next(&unpacker, NULL)) ||
        !qp_is_raw(qp_next(&unpacker, &target)))
    {
        ex_set(e, RQL_PROTO_TYPE_ERR, "invalid event");
        return;
    }

    if (event->target || qpx_obj_eq_str(&target, rql_name)) goto target;

    for (vec_each(event->events->rql->dbs, rql_db_t, db))
    {
        if (qpx_obj_eq_str(&target, db->name) ||
            qpx_obj_eq_str(&target, db->guid.guid))
        {
            event->target = rql_db_grab(db);
            goto target;
        }
    }
    ex_set(e, RQL_PROTO_INDX_ERR,
            "invalid target: '%.*s'", target.len, target.via.raw);
    return;

target:
    event->raw = rql_raw_new(raw, sz);
    if (!event->raw)
    {
        ex_set_alloc(e);
        return;
    }
    rql__event_unpack(event, &unpacker, e);
    return;
}

int rql_event_run(rql_event_t * event)
{
    if (event->status == RQL_EVENT_STAT_CACNCEL) return 0;
    int success = 0;
    rql_task_stat_e rc = RQL_TASK_SUCCESS;
    qp_add_array(&event->result);

    for (vec_each(event->tasks, rql_task_t, task))
    {
        rc = rql_task_run(task, event, rc);
        if (rc == RQL_TASK_ERR) goto failed;
        success += (rc == RQL_TASK_SUCCESS);
    }

    qp_close_array(event->result);

    /* update commit_id */
    event->events->commit_id = event->id;
    log_debug("tasks success: %d (total: %"PRIu32")", success, event->tasks->n);
    return success;

failed:
    log_critical("at least one task failed with a critical error");
    return -1;
}

void rql_event_finish(rql_event_t * event)
{
    rql_pkg_t * pkg;
    if (event->status == RQL_EVENT_STAT_CACNCEL)
    {
        ex_t * e = ex_use();
        ex_set(e, RQL_PROTO_NODE_ERR, "event is cancelled");
        pkg = rql_pkg_err(event->pid, e->nr, e->msg);
        if (!pkg)
        {
            log_error(EX_ALLOC);
            goto stop;
        }
    }
    else
    {
        pkg = qpx_packer_pkg(event->result, RQL_PROTO_RESULT);
        pkg->id = event->pid;
        event->result = NULL;
    }

    if ((event->client) ?
            rql_front_write(event->client, pkg) :
            rql_node_write(event->node, pkg))
    {
        free(pkg);
        log_error(EX_ALLOC);
    }

stop:
    event->events = NULL;  /* prevent looping over queue */
    rql_event_destroy(event);
}

/*
 * Returns 0 when successful and can only return -1 in case the queue required
 * allocation which has failed.
 */
static int rql__event_to_queue(rql_event_t * event)
{
    rql_events_t * events = event->events;
    if (event->id >= events->next_id)
    {
        if (queue_push(&events->queue, event)) return -1;

        events->next_id = event->id + 1;
        return 0;
    }

    size_t i = 0;
    for (queue_each(events->queue, rql_event_t, ev), i++)
    {
        if (ev->id >= event->id) break;
    }

    return queue_insert(&events->queue, i, event);
}

static int rql__event_reg(rql_event_t * event)
{
    rql_t * rql = event->events->rql;
    rql_prom_t * prom = rql_prom_new(
            rql->nodes->n - 1,
            event,
            rql__event_on_reg_cb);
    qpx_packer_t * xpkg = qpx_packer_create(10);
    if (!prom ||
        !xpkg ||
        qp_add_int64(xpkg, (int64_t) event->id)) goto failed;

    event->req_pkg = qpx_packer_pkg(xpkg, RQL_BACK_EVENT_REG);

    for (vec_each(rql->nodes, rql_node_t, node))
    {
        if (node == rql->node) continue;
        if (node->status <= RQL_NODE_STAT_CONNECTED || rql_req(
                node,
                event->req_pkg,
                rql__event_reg_timeout,
                prom,
                rql_prom_req_cb))
        {
            prom->sz--;
            continue;
        }
        assert (event->nodes->n < event->nodes->sz);
        VEC_push(event->nodes, rql_node_grab(node));
    }

    rql_prom_go(prom);

    return 0;

failed:
    free(prom);
    qpx_packer_destroy(xpkg);
    rql_term(SIGTERM);
    return -1;
}

static int rql__event_upd(rql_event_t * event, uint64_t prev_id)
{
    rql_t * rql = event->events->rql;
    rql_prom_t * prom = rql_prom_new(
            event->nodes->n,
            event,
            rql__event_on_reg_cb);
    qpx_packer_t * xpkg = qpx_packer_create(20);
    if (!prom ||
        !xpkg ||
        qp_add_array(&xpkg) ||
        qp_add_int64(xpkg, (int64_t) prev_id) ||
        qp_add_int64(xpkg, (int64_t) event->id) ||
        qp_close_array(xpkg)) goto failed;

    event->req_pkg = qpx_packer_pkg(xpkg, RQL_BACK_EVENT_UPD);

    for (vec_each(event->nodes, rql_node_t, node))
    {
        if (node == rql->node) continue;
        if (node->status <= RQL_NODE_STAT_CONNECTED || rql_req(
                node,
                event->req_pkg,
                rql__event_upd_timeout,
                prom,
                rql_prom_req_cb))
        {
            prom->sz--;
        }
    }

    rql_prom_go(prom);

    return 0;

failed:
    free(prom);
    qpx_packer_destroy(xpkg);
    rql_term(SIGTERM);
    return -1;
}

static void rql__event_on_reg_cb(rql_prom_t * prom)
{
    rql_event_t * event = (rql_event_t *) prom->data;
    free(event->req_pkg);
    _Bool accept = 1;

    for (size_t i = 0; i < prom->n; i++)
    {

        rql_prom_res_t * res = &prom->res[i];
        rql_req_t * req = (rql_req_t *) res->handle;

        if (res->status)
        {
            event->status = RQL_EVENT_STAT_CACNCEL;
        }
        else if (req->pkg_res->tp == RQL_PROTO_REJECT)
        {
            accept = 0;
        }

        rql_req_destroy(req);
    }

    free(prom);

    if (event->status == RQL_EVENT_STAT_CACNCEL)
    {
        // remove from queue if it is still in the queue.
        // event->status could be already set to reject in which case the
        // event is already removed from the queue
        rql__event_cancel(event);  /* terminates in case of failure */
        uv_async_send(&event->events->loop);
        return;
    }

    if (!accept)
    {
        uint64_t old_id = event->id;
        queue_remval(event->events->queue, event);
        event->id = event->events->next_id;
        rql__event_to_queue(event);  /* queue has room */
        rql__event_upd(event, old_id);  /* terminates in case of failure */
        return;
    }

    rql__event_ready(event);  /* terminates in case of failure */
}

static int rql__event_ready(rql_event_t * event)
{
    event->status = RQL_EVENT_STAT_READY;

    rql_t * rql = event->events->rql;

    event->prom = rql_prom_new(
            event->nodes->n + 1,
            event,
            rql__event_on_ready_cb);
    qpx_packer_t * xpkg = qpx_packer_create(20 + event->raw->n);

    if (!event->prom ||
        !xpkg ||
        qp_add_array(&xpkg) ||
        qp_add_int64(xpkg, (int64_t) event->id) ||
        qp_add_raw(xpkg, event->raw->data, event->raw->n) ||
        qp_close_array(xpkg)) goto failed;

    event->req_pkg = qpx_packer_pkg(xpkg, RQL_BACK_EVENT_READY);

    for (vec_each(rql->nodes, rql_node_t, node))
    {
        if (node == rql->node)
        {
            if (uv_async_send(&event->events->loop))
            {
                rql_term(SIGTERM);
                event->prom->sz--;
            }
        }
        else if (node->status <= RQL_NODE_STAT_CONNECTED)
        {
            event->prom->sz--;
        }
        else if (rql_req(
                node,
                event->req_pkg,
                rql__event_ready_timeout,
                event->prom,
                rql_prom_req_cb))
        {
            log_error("failed to write to node: %s", node->addr);
            event->prom->sz--;
        }
    }

    rql_prom_go(event->prom);

    return 0;

failed:
    qpx_packer_destroy(xpkg);
    rql_term(SIGTERM);
    return -1;
}

static void rql__event_on_ready_cb(rql_prom_t * prom)
{
    rql_event_t * event = (rql_event_t *) prom->data;
    free(event->req_pkg);

    for (size_t i = 0; i < prom->n; i++)
    {
        rql_prom_res_t * res = &prom->res[i];

        if (res->tp == RQL_PROM_VIA_ASYNC) continue;

        rql_req_t * req = (rql_req_t *) res->handle;

        if (res->status || req->pkg_res->tp != RQL_PROTO_RESULT)
        {
            log_critical("event failed on node: %s", req->node->addr);
        }

        rql_req_destroy(req);
    }

    free(prom);
    event->prom = NULL;

    rql_event_finish(event);
}

static int rql__event_cancel(rql_event_t * event)
{
    rql_t * rql = event->events->rql;
    rql_prom_t * prom = rql_prom_new(
            event->nodes->n,
            event,
            rql__event_on_cancel_cb);
    qpx_packer_t * xpkg = qpx_packer_create(20);
    if (!prom ||
        !xpkg ||
        qp_add_array(&xpkg) ||
        qp_add_int64(xpkg, (int64_t) event->id) ||
        qp_add_int64(xpkg, (int64_t) event->node->id) ||
        qp_close_array(xpkg)) goto failed;

    rql_pkg_t * pkg = qpx_packer_pkg(xpkg, RQL_BACK_EVENT_CANCEL);

    for (vec_each(event->nodes, rql_node_t, node))
    {
        if (node == rql->node) continue;
        if (node->status <= RQL_NODE_STAT_CONNECTED || rql_req(
                node,
                pkg,
                rql__event_cancel_timeout,
                prom,
                rql_prom_req_cb))
        {
            prom->sz--;
        }
    }

    free(prom->sz ? NULL : pkg);
    rql_prom_go(prom);

    return 0;

failed:
    free(prom);
    qpx_packer_destroy(xpkg);
    rql_term(SIGTERM);
    return -1;
}

static void rql__event_on_cancel_cb(rql_prom_t * prom)
{
    for (size_t i = 0; i < prom->n; i++)
    {
        rql_prom_res_t * res = &prom->res[i];
        rql_req_t * req = (rql_req_t *) res->handle;
        free(i ? NULL : req->pkg_req);
        rql_req_destroy(req);
    }
    free(prom);
}


/*
 * Returns 0 when tested successful
 */
//int rql_event_accept_id(rql_event_t * event)
//{
//    assert (event->id != NULL);
//    assert (event->node != NULL);
//    if (event->id > *rql__event_max_id(event)) goto accept;
//    for (link_each(event->rql->queue, rql_event_t, ev))
//    {
//        if (ev->id > event->id || ev->target != event->target) continue;
//        if (ev->id < event->id) goto accept;
//        if (rql__event_cmp(event, ev) < 0) return -1;
//
//        assert (ev->status <= RQL_EVENT_STAT_WAIT_ACCEPT);
//
//        link_pop_current(event->rql->queue);
//        rql_event_destroy(ev);
//
//
//    }
//accept:
//    event->rql->event_max_id = event->id;
//    return 0;
//}

/*
 * Compare two events with the same id.
 */
//inline static int rql__event_cmp(rql_event_t * a, rql_event_t * b)
//{
//    assert (a->id == b->id && a->node->id != b->node->id);
//    return ((a->node->id + a->id) % a->events->rql->nodes->n) -
//           ((b->node->id + b->id) % b->events->rql->nodes->n);
//}

static void rql__event_unpack(
        rql_event_t * event,
        qp_unpacker_t * unpacker,
        ex_t * e)
{
    qp_res_t * res;
    if (!qp_is_array(qp_next(unpacker, NULL)))
    {
        ex_set(e, RQL_PROTO_TYPE_ERR,
                "invalid event: expecting an array with tasks");
        return;
    }

    while ((res = qp_unpacker_res(unpacker, NULL)) && res->tp == QP_RES_MAP)
    {
        rql_task_t * task = rql_task_create(res, e);
        if (!task)
        {
            qp_res_destroy(res);
            return;  /* e is set */
        }

        if (vec_push(&event->tasks, task))
        {
            rql_task_destroy(task);
            ex_set_alloc(e);
            return;
        }

        if (event->client &&
            !rql_access_check(
                (event->target) ?
                        event->target->access : event->events->rql->access,
                event->client->via.user,
                task->tp))
        {
            ex_set(e, RQL_PROTO_AUTH_ERR,
                    "user has no privileges for task: %s",
                    rql_task_str(task));
            return;
        }
    }

    if (unpacker->pt != unpacker->end)
    {
        ex_set(e, RQL_PROTO_TYPE_ERR, "error unpacking tasks from event");
        return;
    }
}

