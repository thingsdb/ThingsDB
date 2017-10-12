/*
 * event.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <rql/event.h>
#include <qpack.h>
#include <util/link.h>
#include <util/ex.h>

inline static int rql__event_cmp(rql_event_t * a, rql_event_t * b);
inline static uint64_t * rql__event_cur_id(rql_event_t * event);
inline static uint64_t * rql__event_max_id(rql_event_t * event);

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

void rql_event_init(rql_event_t * event, rql_db_t * target)
{
    event->target = (target) ? rql_db_grab(target) : NULL;
    event->node = rql_node_grab(event->rql->node);
    event->id = ++(*rql__event_max_id(event));
}

void rql_event_set(
        rql_event_t * event,
        uint64_t id,
        rql_node_t * node,
        rql_db_t * target)
{
    event->target = (target) ? rql_db_grab(target) : NULL;
    event->node = rql_node_grab(node);
    event->id = id;
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
        ex_set(e, RQL_FRONT_TYPE_ERR, "invalid event");  /* TODO: add info */
        return -1;
    }

    if (event->target || qpx_raw_equal(&target, "_rql")) goto target;

    for (link_each(event->rql->dbs, rql_db_t, db))
    {
        if (qpx_raw_equal(&target, db->name))
        {
            event->target = rql_db_grab(db);
            goto target;
        }
    }
    ex_set(e, RQL_FRONT_INDX_ERR,
            "invalid target: '%.*s'", target.len, target.via.raw);
    return -1;

target:
    event->raw = (unsigned char *) malloc(sz);
    if (!event->raw)
    {
        ex_set(e, RQL_FRONT_RUNT_ERR, EX_ALLOC);
        return -1;
    }

    memcpy(event->raw, raw, sz);
    event->raw_sz = sz;

    return 0;
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

inline static uint64_t * rql__event_cur_id(rql_event_t * event)
{
    return (event->target) ?
            &event->target->event_cur_id : &event->rql->event_cur_id;
}

inline static uint64_t * rql__event_max_id(rql_event_t * event)
{
    return (event->target) ?
            &event->target->event_max_id : &event->rql->event_max_id;
}
