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
#include <rql/proto.h>
#include <rql/task.h>
#include <util/link.h>
#include <util/queue.h>
#include <util/ex.h>
#include <util/qpx.h>

const int rql_event_reg_timeout = 10;

static void rql__event_on_reg_cb(rql_prom_t * prom);
inline static int rql__event_cmp(rql_event_t * a, rql_event_t * b);
static int rql__event_unpack(
        rql_event_t * event,
        qp_unpacker_t * unpacker,
        ex_t * e);

rql_event_t * rql_event_create(rql_t * rql)
{
    rql_event_t * event = malloc(sizeof(rql_event_t));
    if (!event) return NULL;
    event->rql = rql;
    event->target = NULL;
    event->node = NULL;
    event->raw = NULL;
    event->source = NULL;
    event->refelems = NULL;
    event->tasks = vec_new(1);
    event->result = qpx_packer_create(16);

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
    rql_db_drop(event->target);
    rql_node_drop(event->node);
    rql_sock_drop(event->source);
    vec_destroy(event->tasks, (vec_destroy_cb) qp_res_destroy);
    if (event->result)
    {
        qp_packer_destroy(event->result);
    }
    free(event->raw);
    free(event);
}

void rql_event_init(rql_event_t * event)
{
    event->node = rql_node_grab(event->rql->node);
    event->id = event->rql->event_next_id;
    event->status = RQL_EVENT_STAT_WAIT_ACCEPT;
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

int rql_event_raw(
        rql_event_t * event,
        const unsigned char * raw,
        size_t sz,
        ex_t * e)
{
    qp_obj_t target;
    qp_unpacker_t unpacker;
    qp_unpacker_init(&unpacker, raw, sz);
    if (!qp_is_map(qp_next(&unpacker, NULL)) ||
        !qp_is_raw(qp_next(&unpacker, &target)))
    {
        ex_set(e, RQL_PROTO_TYPE_ERR, "invalid event");  /* TODO: add info */
        return -1;
    }

    if (event->target || qpx_raw_equal(&target, rql_name)) goto target;

    link_iter_t iter = link_iter(event->rql->dbs);
    for (link_each(iter, rql_db_t, db))
    {
        if (qpx_raw_equal(&target, db->name))
        {
            event->target = rql_db_grab(db);
            goto target;
        }
    }
    ex_set(e, RQL_PROTO_INDX_ERR,
            "invalid target: '%.*s'", target.len, target.via.raw);
    return -1;

target:
    event->raw = rql_raw_new(raw, sz);
    if (!event->raw)
    {
        ex_set(e, RQL_PROTO_RUNT_ERR, EX_ALLOC);
        return -1;
    }
    LOGC("Bla....5");
    return rql__event_unpack(event, &unpacker, e);
}

int rql_event_run(rql_event_t * event)
{
    int success = 0;
    rql_task_stat_e rc = RQL_TASK_SUCCESS;
    qp_add_array(&event->result);
    for (vec_each(event->tasks, qp_res_t, task))
    {
        rc = rql_task(task, event, rc);
        if (rc == RQL_TASK_ERR) goto failed;
        success += (rc == RQL_TASK_SUCCESS);
    }

    qp_close_array(event->result);

    return success;

failed:
    return -1;
}

int rql_event_done(rql_event_t * event)
{
    if (vec_push(&event->rql->done, event->raw)) return -1;
    event->raw = NULL;
    return 0;
}

int rql_event_to_queue(rql_event_t * event)
{
    rql_t * rql = event->rql;
    if (event->id >= rql->event_next_id)
    {
        if (queue_push(&rql->queue, event)) return -1;

        rql->event_next_id = event->id + 1;
        return 0;
    }

    size_t i = 0;
    for (queue_each(rql->queue, rql_event_t, ev), i++)
    {
        if (ev->id >= event->id) break;
    }

    return queue_insert(&rql->queue, i, event);
}

int rql_event_reg(rql_event_t * event)
{
    rql_t * rql = event->rql;

    rql_prom_t * prom = rql_prom_new(
            rql->nodes->n - 1,
            event,
            rql__event_on_reg_cb);

    qpx_packer_t * xpkg = qpx_packer_create(20);

    if (qp_add_array(&xpkg) ||
        qp_add_int64(xpkg, (int64_t) event->id) ||
        qp_add_int64(xpkg, (int64_t) event->node->id) ||
        qp_close_array(xpkg)) goto failed;

    rql_pkg_t * pkg = qpx_packer_pkg(xpkg, RQL_BACK_EVENT_REG);

    for (vec_each(rql->nodes, rql_node_t, node))
    {
        if (node == rql->node) continue;
        if (node->status != RQL_NODE_STAT_READY || rql_req(
                node,
                pkg,
                rql_event_reg_timeout,
                prom,
                rql_prom_req_cb))
        {
            prom->sz--;
        }
    }

    free(prom->sz ? NULL : pkg);
    rql_prom_go(prom);

failed:
    return -1;
}

static void rql__event_on_reg_cb(rql_prom_t * prom)
{
    rql_event_t * event = (rql_event_t *) prom->data;

    for (size_t i = 0; i < prom->n; i++)
    {

        rql_prom_res_t * res = &prom->res[i];
        rql_req_t * req = (rql_req_t *) res->handle;
        free(i ? NULL : req->pkg_req);

        if (res->status)
        {
            // send failed event to client  RQL_PROTO_NODE_ERR
        }

        if (req->pkg_res->tp == RQL_PROTO_REJECT)
        {
            event->status = RQL_EVENT_STAT_REJECTED;

        }

        rql_req_destroy(req);
    }
    if (event->status == RQL_EVENT_STAT_REJECTED)
    {
        // remove from queue if it is still in the queue.
        // event->status could be already set to reject in which case the
        // event is already removed from the queue
    }
    else
    {
        // event go...
    }
    free(prom);
}

static int rql__event_go(rql_event_t * event)
{
    event->status = RQL_EVENT_STAT_ACCEPTED;

    rql_t * rql = event->rql;

    rql_prom_t * prom = rql_prom_new(
            rql->nodes->n,
            event,
            rql__event_on_reg_cb);

    qpx_packer_t * xpkg = qpx_packer_create(20 + event->raw->n);

    if (qp_add_array(&xpkg) ||
        qp_add_int64(xpkg, (int64_t) event->id) ||
        qp_add_int64(xpkg, (int64_t) event->node->id) ||
        qp_add_raw(xpkg, (const char *) event->raw->data, event->raw->n) ||
        qp_close_array(xpkg)) goto failed;

    rql_pkg_t * pkg = qpx_packer_pkg(xpkg, RQL_BACK_EVENT_GO);

    for (vec_each(rql->nodes, rql_node_t, node))
    {
        if (node == rql->node)
        {
            uv_async_send(&rql->event_loop);
            continue;
        }
        if (node->status != RQL_NODE_STAT_READY || rql_req(
                node,
                pkg,
                rql_event_reg_timeout,
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
    return -1;
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
inline static int rql__event_cmp(rql_event_t * a, rql_event_t * b)
{
    assert (a->id == b->id && a->rql->nodes->n == b->rql->nodes->n);
    return ((a->node->id + a->id) % a->rql->nodes->n) -
           ((b->node->id + b->id) % b->rql->nodes->n);
}

static int rql__event_unpack(
        rql_event_t * event,
        qp_unpacker_t * unpacker,
        ex_t * e)
{
    qp_unpacker_t tmp;
    qp_res_t * res;
    if (!qp_is_array(qp_next(unpacker, NULL)))
    {
        ex_set(e, RQL_PROTO_TYPE_ERR,
                "invalid event: expecting an array with tasks");
        return -1;
    }
//    qp_unpacker_init(&tmp, unpacker->pt)
    while ((res = qp_unpacker_res(unpacker, NULL)) && res->tp == QP_RES_MAP)
    {
        if (vec_push(&event->tasks, res))
        {
            ex_set(e, RQL_PROTO_RUNT_ERR, "allocation error");
            return -1;
        }
    }

    if (unpacker->pt != unpacker->end)
    {
        ex_set(e, RQL_PROTO_TYPE_ERR, "error unpacking tasks from event");
        return -1;
    }
    return 0;
}

