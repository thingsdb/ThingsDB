/*
 * event.c
 */
#include <assert.h>
#include <nodes.h>
#include <stdlib.h>
#include <qpack.h>
#include <thingsdb.h>
#include <ti/access.h>
#include <ti/dbs.h>
#include <ti/event.h>
#include <ti/proto.h>
#include <ti/req.h>
#include <ti/task.h>
#include <util/ex.h>
#include <util/link.h>
#include <util/qpx.h>
#include <util/queue.h>

const int ti__event_reg_timeout = 10;
const int ti__event_upd_timeout = 10;
const int ti__event_ready_timeout = 30;
const int ti__event_cancel_timeout = 5;

static int ti__event_to_queue(ti_event_t * event);
static int ti__event_reg(ti_event_t * event);
static int ti__event_upd(ti_event_t * event, uint64_t prev_id);
static void ti__event_on_reg_cb(ti_prom_t * prom);
static int ti__event_ready(ti_event_t * event);
static void ti__event_on_ready_cb(ti_prom_t * prom);
static int ti__event_cancel(ti_event_t * event);
static void ti__event_on_cancel_cb(ti_prom_t * prom);
//inline static int ti__event_cmp(ti_event_t * a, ti_event_t * b);
static void ti__event_unpack(
        ti_event_t * event,
        qp_unpacker_t * unpacker,
        ex_t * e);

ti_event_t * ti_event_create(ti_events_t * events)
{
    ti_event_t * event = malloc(sizeof(ti_event_t));
    if (!event) return NULL;
    event->events = events;
    event->target = NULL;
    event->node = NULL;
    event->raw = NULL;
    event->client = NULL;
    event->refthings = NULL;
    event->tasks = vec_new(1);
    event->result = qpx_packer_create(16);
    event->status = TI_EVENT_STAT_UNINITIALIZED;
    event->nodes = NULL;
    event->prom = NULL;

    if (!event->tasks)
    {
        ti_event_destroy(event);
        return NULL;
    }

    return event;
}

void ti_event_destroy(ti_event_t * event)
{
    if (!event) return;
    if (event->events)
    {
        /* the event might be in the queue */
        queue_remval(event->events->queue, event);
    }
    ti_db_drop(event->target);
    ti_node_drop(event->node);
    ti_sock_drop(event->client);
    imap_destroy(event->refthings, NULL);
    vec_destroy(event->tasks, (vec_destroy_cb) ti_task_destroy);
    vec_destroy(event->nodes, (vec_destroy_cb) ti_node_drop);
    qpx_packer_destroy(event->result);
    free(event->prom);
    free(event->raw);
    free(event);
}

void ti_event_new(ti_sock_t * sock, ti_pkg_t * pkg, ex_t * e)
{
    thingsdb_t * thingsdb = thingsdb_get();
    ti_event_t * event = ti_event_create(thingsdb->events);
    if (!event || !(event->nodes = vec_new(thingsdb->nodes->n - 1)))
    {
        ex_set_alloc(e);
        goto failed;
    }

    event->id = event->events->next_id;
    event->pid = pkg->id;
    event->node = ti_node_grab(thingsdb->node);
    event->client = ti_sock_grab(sock);
    event->status = TI_EVENT_STAT_REG;

    if (!ti_nodes_has_quorum(thingsdb->nodes))
    {
        ex_set(e, TI_PROTO_NODE_ERR,
                "node '%s' does not have the required quorum",
                thingsdb->node->addr);
        goto failed;
    }

    ti_event_raw(event, pkg->data, pkg->n, e);
    if (e->nr) goto failed;

    if (ti__event_to_queue(event) || ti__event_reg(event))
    {
        ex_set_alloc(e);
        goto failed;
    }

    return;  /* success */

failed:
    assert (e->nr);
    ti_event_destroy(event);
}

int ti_event_init(ti_event_t * event)
{
    thingsdb_t * thingsdb = thingsdb_get();
    event->node = ti_node_grab(thingsdb->node);
    event->id = event->events->next_id;
    event->status = TI_EVENT_STAT_REG;
    event->nodes = vec_new(thingsdb->nodes->n - 1);
    return -!event->nodes;
}

//int ti_event_fill(
//        ti_event_t * event,
//        uint64_t id,
//        ti_node_t * node,
//        ti_db_t * target)
//{
//    ti_t * tin = event->tin;
//    if (id > tin->event_max_id)
//    {
//        if (link_unshift(tin->queue, event))
//        {
//            log_error(EX_ALLOC);
//            return -1;
//        }
//        tin->event_max_id = id;;
//        goto accept;
//    }
//
//    if (id <= tin->event_cur_id)
//    {
//        log_warning("reject event id: %"PRIu64" (current id: %"PRIu64")",
//                id, tin->event_cur_id);
//        return -1;
//    }
//
//    for (link_each(event->tin->queue, ti_event_t, ev))
//    {
//        if (id ev->target != target) continue;
//        assert (ev->id > event->id)
//        if (ev->id < event->id) goto accept;
//        if (ti__event_cmp(event, ev) < 0) return -1;
//
//        assert (ev->status <= TI_EVENT_STAT_WAIT_ACCEPT);
//
//        link_pop_current(event->tin->queue);
//        ti_event_destroy(ev);
//    }
//
//accept:
//    //    *ti__event_max_id(event) = event->id;
//    event->id = id;
//    event->target = (target) ? ti_db_grab(target) : NULL;
//    event->node = ti_node_grab(node);
//    return 0;
//}

void ti_event_raw(
        ti_event_t * event,
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
        ex_set(e, TI_PROTO_TYPE_ERR, "invalid event");
        return;
    }

    if (event->target || qpx_obj_eq_str(&target, "_")) goto target;

    event->target = ti_dbs_get_by_obj(thingsdb_get()->dbs, &target);
    if (event->target)
    {
        event->target = ti_db_grab(event->target);
        goto target;
    }

    ex_set(e, TI_PROTO_INDX_ERR,
            "invalid target: '%.*s'", target.len, target.via.raw);
    return;

target:
    event->raw = ti_raw_new(raw, sz);
    if (!event->raw)
    {
        ex_set_alloc(e);
        return;
    }
    ti__event_unpack(event, &unpacker, e);
    return;
}

int ti_event_run(ti_event_t * event)
{
    if (event->status == TI_EVENT_STAT_CACNCEL) return 0;
    int success = 0;
    ti_task_stat_e rc = TI_TASK_SUCCESS;
    qp_add_array(&event->result);

    for (vec_each(event->tasks, ti_task_t, task))
    {
        rc = ti_task_run(task, event, rc);
        if (rc == TI_TASK_ERR) goto failed;
        success += (rc == TI_TASK_SUCCESS);
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

void ti_event_finish(ti_event_t * event)
{
    ti_pkg_t * pkg;
    if (event->status == TI_EVENT_STAT_CACNCEL)
    {
        ex_t * e = ex_use();
        ex_set(e, TI_PROTO_NODE_ERR, "event is cancelled");
        pkg = ti_pkg_err(event->pid, e->nr, e->msg);
        if (!pkg)
        {
            log_error(EX_ALLOC);
            goto stop;
        }
    }
    else
    {
        pkg = qpx_packer_pkg(event->result, TI_PROTO_RESULT);
        pkg->id = event->pid;
        event->result = NULL;
    }

    if ((event->client) ?
            ti_front_write(event->client, pkg) :
            ti_node_write(event->node, pkg))
    {
        free(pkg);
        log_error(EX_ALLOC);
    }

stop:
    event->events = NULL;  /* prevent looping over queue */
    ti_event_destroy(event);
}

/*
 * Returns 0 when successful and can only return -1 in case the queue required
 * allocation which has failed.
 */
static int ti__event_to_queue(ti_event_t * event)
{
    ti_events_t * events = event->events;
    if (event->id >= events->next_id)
    {
        if (queue_push(&events->queue, event)) return -1;

        events->next_id = event->id + 1;
        return 0;
    }

    size_t i = 0;
    for (queue_each(events->queue, ti_event_t, ev), i++)
    {
        if (ev->id >= event->id) break;
    }

    return queue_insert(&events->queue, i, event);
}

static int ti__event_reg(ti_event_t * event)
{
    thingsdb_t * thingsdb = thingsdb_get();
    ti_prom_t * prom = ti_prom_new(
            thingsdb->nodes->n - 1,
            event,
            ti__event_on_reg_cb);
    qpx_packer_t * xpkg = qpx_packer_create(10);
    if (!prom ||
        !xpkg ||
        qp_add_int64(xpkg, (int64_t) event->id)) goto failed;

    event->req_pkg = qpx_packer_pkg(xpkg, TI_BACK_EVENT_REG);

    for (vec_each(thingsdb->nodes, ti_node_t, node))
    {
        if (node == thingsdb->node) continue;
        if (node->status <= TI_NODE_STAT_CONNECTED || ti_req(
                node,
                event->req_pkg,
                ti__event_reg_timeout,
                prom,
                ti_prom_req_cb))
        {
            prom->sz--;
            continue;
        }
        assert (event->nodes->n < event->nodes->sz);
        VEC_push(event->nodes, ti_node_grab(node));
    }

    ti_prom_go(prom);

    return 0;

failed:
    free(prom);
    qpx_packer_destroy(xpkg);
    ti_term(SIGTERM);
    return -1;
}

static int ti__event_upd(ti_event_t * event, uint64_t prev_id)
{
    thingsdb_t * thingsdb = thingsdb_get();
    ti_prom_t * prom = ti_prom_new(
            event->nodes->n,
            event,
            ti__event_on_reg_cb);
    qpx_packer_t * xpkg = qpx_packer_create(20);
    if (!prom ||
        !xpkg ||
        qp_add_array(&xpkg) ||
        qp_add_int64(xpkg, (int64_t) prev_id) ||
        qp_add_int64(xpkg, (int64_t) event->id) ||
        qp_close_array(xpkg)) goto failed;

    event->req_pkg = qpx_packer_pkg(xpkg, TI_BACK_EVENT_UPD);

    for (vec_each(event->nodes, ti_node_t, node))
    {
        if (node == thingsdb->node) continue;
        if (node->status <= TI_NODE_STAT_CONNECTED || ti_req(
                node,
                event->req_pkg,
                ti__event_upd_timeout,
                prom,
                ti_prom_req_cb))
        {
            prom->sz--;
        }
    }

    ti_prom_go(prom);

    return 0;

failed:
    free(prom);
    qpx_packer_destroy(xpkg);
    ti_term(SIGTERM);
    return -1;
}

static void ti__event_on_reg_cb(ti_prom_t * prom)
{
    ti_event_t * event = (ti_event_t *) prom->data;
    free(event->req_pkg);
    _Bool accept = 1;

    for (size_t i = 0; i < prom->n; i++)
    {

        ti_prom_res_t * res = &prom->res[i];
        ti_req_t * req = (ti_req_t *) res->handle;

        if (res->status)
        {
            event->status = TI_EVENT_STAT_CACNCEL;
        }
        else if (req->pkg_res->tp == TI_PROTO_REJECT)
        {
            accept = 0;
        }

        ti_req_destroy(req);
    }

    free(prom);

    if (event->status == TI_EVENT_STAT_CACNCEL)
    {
        // remove from queue if it is still in the queue.
        // event->status could be already set to reject in which case the
        // event is already removed from the queue
        ti__event_cancel(event);  /* terminates in case of failure */
        uv_async_send(&event->events->loop);
        return;
    }

    if (!accept)
    {
        uint64_t old_id = event->id;
        queue_remval(event->events->queue, event);
        event->id = event->events->next_id;
        ti__event_to_queue(event);  /* queue has room */
        ti__event_upd(event, old_id);  /* terminates in case of failure */
        return;
    }

    ti__event_ready(event);  /* terminates in case of failure */
}

static int ti__event_ready(ti_event_t * event)
{
    thingsdb_t * thingsdb = thingsdb_get();
    event->status = TI_EVENT_STAT_READY;

    event->prom = ti_prom_new(
            event->nodes->n + 1,
            event,
            ti__event_on_ready_cb);
    qpx_packer_t * xpkg = qpx_packer_create(20 + event->raw->n);

    if (!event->prom ||
        !xpkg ||
        qp_add_array(&xpkg) ||
        qp_add_int64(xpkg, (int64_t) event->id) ||
        qp_add_raw(xpkg, event->raw->data, event->raw->n) ||
        qp_close_array(xpkg)) goto failed;

    event->req_pkg = qpx_packer_pkg(xpkg, TI_BACK_EVENT_READY);

    for (vec_each(thingsdb->nodes, ti_node_t, node))
    {
        if (node == thingsdb->node)
        {
            if (uv_async_send(&event->events->loop))
            {
                ti_term(SIGTERM);
                event->prom->sz--;
            }
        }
        else if (node->status <= TI_NODE_STAT_CONNECTED)
        {
            event->prom->sz--;
        }
        else if (ti_req(
                node,
                event->req_pkg,
                ti__event_ready_timeout,
                event->prom,
                ti_prom_req_cb))
        {
            log_error("failed to write to node: `%s`", node->addr);
            event->prom->sz--;
        }
    }

    ti_prom_go(event->prom);

    return 0;

failed:
    qpx_packer_destroy(xpkg);
    ti_term(SIGTERM);
    return -1;
}

static void ti__event_on_ready_cb(ti_prom_t * prom)
{
    ti_event_t * event = (ti_event_t *) prom->data;
    free(event->req_pkg);

    for (size_t i = 0; i < prom->n; i++)
    {
        ti_prom_res_t * res = &prom->res[i];

        if (res->tp == TI_PROM_VIA_ASYNC) continue;

        ti_req_t * req = (ti_req_t *) res->handle;

        if (res->status || req->pkg_res->tp != TI_PROTO_RESULT)
        {
            log_critical("event failed on node: `%s`", req->node->addr);
        }

        ti_req_destroy(req);
    }

    free(prom);
    event->prom = NULL;

    ti_event_finish(event);
}

static int ti__event_cancel(ti_event_t * event)
{
    thingsdb_t * thingsdb = thingsdb_get();
    ti_prom_t * prom = ti_prom_new(
            event->nodes->n,
            event,
            ti__event_on_cancel_cb);
    qpx_packer_t * xpkg = qpx_packer_create(20);
    if (!prom ||
        !xpkg ||
        qp_add_array(&xpkg) ||
        qp_add_int64(xpkg, (int64_t) event->id) ||
        qp_add_int64(xpkg, (int64_t) event->node->id) ||
        qp_close_array(xpkg)) goto failed;

    ti_pkg_t * pkg = qpx_packer_pkg(xpkg, TI_BACK_EVENT_CANCEL);

    for (vec_each(event->nodes, ti_node_t, node))
    {
        if (node == thingsdb->node) continue;
        if (node->status <= TI_NODE_STAT_CONNECTED || ti_req(
                node,
                pkg,
                ti__event_cancel_timeout,
                prom,
                ti_prom_req_cb))
        {
            prom->sz--;
        }
    }

    free(prom->sz ? NULL : pkg);
    ti_prom_go(prom);

    return 0;

failed:
    free(prom);
    qpx_packer_destroy(xpkg);
    ti_term(SIGTERM);
    return -1;
}

static void ti__event_on_cancel_cb(ti_prom_t * prom)
{
    for (size_t i = 0; i < prom->n; i++)
    {
        ti_prom_res_t * res = &prom->res[i];
        ti_req_t * req = (ti_req_t *) res->handle;
        free(i ? NULL : req->pkg_req);
        ti_req_destroy(req);
    }
    free(prom);
}


/*
 * Returns 0 when tested successful
 */
//int ti_event_accept_id(ti_event_t * event)
//{
//    assert (event->id != NULL);
//    assert (event->node != NULL);
//    if (event->id > *ti__event_max_id(event)) goto accept;
//    for (link_each(event->tin->queue, ti_event_t, ev))
//    {
//        if (ev->id > event->id || ev->target != event->target) continue;
//        if (ev->id < event->id) goto accept;
//        if (ti__event_cmp(event, ev) < 0) return -1;
//
//        assert (ev->status <= TI_EVENT_STAT_WAIT_ACCEPT);
//
//        link_pop_current(event->tin->queue);
//        ti_event_destroy(ev);
//
//
//    }
//accept:
//    event->tin->event_max_id = event->id;
//    return 0;
//}

/*
 * Compare two events with the same id.
 */
//inline static int ti__event_cmp(ti_event_t * a, ti_event_t * b)
//{
//    assert (a->id == b->id && a->node->id != b->node->id);
//    return ((a->node->id + a->id) % a->events->tin->nodes->n) -
//           ((b->node->id + b->id) % b->events->tin->nodes->n);
//}

static void ti__event_unpack(
        ti_event_t * event,
        qp_unpacker_t * unpacker,
        ex_t * e)
{
    qp_res_t * res;
    thingsdb_t * thingsdb = thingsdb_get();
    if (!qp_is_array(qp_next(unpacker, NULL)))
    {
        ex_set(e, TI_PROTO_TYPE_ERR,
                "invalid event: `expecting an array with tasks`");
        return;
    }

    while ((res = qp_unpacker_res(unpacker, NULL)) && res->tp == QP_RES_MAP)
    {
        ti_task_t * task = ti_task_create(res, e);
        if (!task)
        {
            qp_res_destroy(res);
            return;  /* e is set */
        }

        if (vec_push(&event->tasks, task))
        {
            ti_task_destroy(task);
            ex_set_alloc(e);
            return;
        }

        if (event->client &&
            !ti_access_check(
                (event->target) ?
                        event->target->access : thingsdb->access,
                event->client->via.user,
                (uint64_t) 1 << task->tp))
        {
            ex_set(e, TI_PROTO_AUTH_ERR,
                    "user has no privileges for task: `%s`",
                    ti_task_str(task));
            return;
        }
    }

    if (unpacker->pt != unpacker->end)
    {
        ex_set(e, TI_PROTO_TYPE_ERR, "error unpacking tasks from event");
        return;
    }
}

