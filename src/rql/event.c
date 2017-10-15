/*
 * event.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <rql/event.h>
#include <qpack.h>
#include <util/link.h>
#include <util/queue.h>
#include <util/ex.h>

const int rql_event_approval_timeout = 10;

inline static int rql__event_cmp(rql_event_t * a, rql_event_t * b);

rql_event_t * rql_event_create(rql_t * rql)
{
    rql_event_t * event = malloc(sizeof(rql_event_t));
    if (!event) return NULL;
    event->rql = rql;
    event->target = NULL;
    event->node = NULL;
    event->raw = NULL;
    return event;
}

void rql_event_destroy(rql_event_t * event)
{
    if (!event) return;
    rql_db_drop(event->target);
    rql_node_drop(event->node);
    free(event->raw);
    free(event);
}

void rql_event_init(rql_event_t * event)
{
    event->node = rql_node_grab(event->rql->node);
    event->id = event->rql->event_next_id;
    event->status = RQL_EVENT_STATUS_WAIT_ACCEPT;
}

int rql_event_fill(
        rql_event_t * event,
        uint64_t id,
        rql_node_t * node,
        rql_db_t * target)
{
    rql_t * rql = event->rql;
    if (id > rql->event_max_id)
    {
        if (link_unshift(rql->queue, event))
        {
            log_error(EX_ALLOC);
            return -1;
        }
        rql->event_max_id = id;;
        goto accept;
    }

    if (id <= rql->event_cur_id)
    {
        log_warning("reject event id: %"PRIu64" (current id: %"PRIu64")",
                id, rql->event_cur_id);
        return -1;
    }

    for (link_each(event->rql->queue, rql_event_t, ev))
    {
        if (id ev->target != target) continue;
        assert (ev->id > event->id)
        if (ev->id < event->id) goto accept;
        if (rql__event_cmp(event, ev) < 0) return -1;

        assert (ev->status <= RQL_EVENT_STATUS_WAIT_ACCEPT);

        link_pop_current(event->rql->queue);
        rql_event_destroy(ev);
    }

accept:
    //    *rql__event_max_id(event) = event->id;
    event->id = id;
    event->target = (target) ? rql_db_grab(target) : NULL;
    event->node = rql_node_grab(node);
    return 0;
}

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
        !qp_is_raw(qp_next(&unpacker, target)))
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
    event->raw = (unsigned char *) malloc(sz);
    if (!event->raw)
    {
        ex_set(e, RQL_PROTO_RUNT_ERR, EX_ALLOC);
        return -1;
    }

    memcpy(event->raw, raw, sz);
    event->raw_sz = sz;

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

int rql_event_get_approval(rql_event_t * event)
{
    rql_t * rql = event->rql;
    char * target = (event->target) ? event->target->name : rql_name;

    rql_prom_t * prom = rql_prom_create(
            rql->nodes->n - 1,
            event,
            rql__event_on_approbal_cb);

    qpx_packer_t * xpkg = qpx_packer_create(64);

    if (qp_add_array(&xpkg) ||
        qp_add_int64(xpkg, (int64_t) event->id) ||
        qp_add_int64(xpkg, (int64_t) event->node->id) ||
        qp_add_raw(xpkg, target, strlen(target)) ||
        qp_close_array(xpkg)) goto failed;

    rql_pkg_t * pkg = qpx_packer_pkg(xpkg, RQL_BACK_REG_EVENT);

    for (vec_each(rql->nodes, rql_node_t, node))
    {
        if (node == rql->node) continue;
        if (node->status != RQL_NODE_STAT_READY || rql_req(node,
                pkg,
                rql_event_approval_timeout,
                rql_prom_grab(prom),
                rql_prom_req_cb))
        {
            prom->sz--;
        }
    }

    if (!prom->sz)
    {
        free(pkg);
    }

    rql_prom_go(prom);

failed:
    return -1;
}

void rql__event_on_approbal_cb(rql_prom_t * prom)
{
    for (size_t i = 0; i < prom->n; i++)
    {
        rql_req_t * req = (rql_req_t *) prom->res[i].handle;

    }
}

/*
 * Returns 0 when tested successful
 */
int rql_event_accept_id(rql_event_t * event)
{
    assert (event->id != NULL);
    assert (event->node != NULL);
    if (event->id > *rql__event_max_id(event)) goto accept;
    for (link_each(event->rql->queue, rql_event_t, ev))
    {
        if (ev->id > event->id || ev->target != event->target) continue;
        if (ev->id < event->id) goto accept;
        if (rql__event_cmp(event, ev) < 0) return -1;

        assert (ev->status <= RQL_EVENT_STATUS_WAIT_ACCEPT);

        link_pop_current(event->rql->queue);
        rql_event_destroy(ev);


    }
accept:
    event->rql->event_max_id = event->id;
    return 0;
}

/*
 * Compare two events with the same id.
 */
inline static int rql__event_cmp(rql_event_t * a, rql_event_t * b)
{
    assert (a->id == b->id && a->rql->nodes->n == b->rql->nodes->n);
    return ((a->node->id + a->id) % a->rql->nodes->n) -
           ((b->node->id + b->id) % b->rql->nodes->n);
}


