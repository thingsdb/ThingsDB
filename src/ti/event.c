/*
 * ti/event.c
 */
#include <assert.h>
#include <ex.h>
#include <qpack.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/collection.h>
#include <ti/job.h>
#include <ti/rjob.h>
#include <ti/event.h>
#include <ti/collections.h>
#include <ti/node.h>
#include <ti/proto.h>
#include <ti/task.h>
#include <util/omap.h>
#include <util/logger.h>
#include <util/qpx.h>


ti_event_t * ti_event_create(ti_event_tp_enum tp)
{
    ti_event_t * ev = malloc(sizeof(ti_event_t));
    if (!ev)
        return NULL;

    ev->ref = 1;
    ev->status = TI_EVENT_STAT_NEW;
    ev->collection = NULL;
    ev->tp = tp;
    ev->flags = 0;
    ev->_tasks = tp == TI_EVENT_TP_MASTER ? vec_new(1) : NULL;

    if (    (tp == TI_EVENT_TP_MASTER && !ev->_tasks) ||
            clock_gettime(TI_CLOCK_MONOTONIC, &ev->time))
    {
        free(ev);
        return NULL;
    }

    return ev;
}

ti_event_t * ti_event_initial(void)
{
    ti_event_t * ev;
    ti_epkg_t * epkg = ti_epkg_initial();
    if (!epkg)
        return NULL;
    ev = ti_event_create(TI_EVENT_TP_EPKG);
    if (!ev)
    {
        ti_epkg_drop(epkg);
        return NULL;
    }

    ev->status = TI_EVENT_STAT_READY;
    ev->via.epkg = epkg;
    ev->id = epkg->event_id;

    return ev;
}

void ti_event_drop(ti_event_t * ev)
{
    if (!ev || --ev->ref)
        return;

    ti_collection_drop(ev->collection);

    if (ev->tp == TI_EVENT_TP_SLAVE)
        ti_node_drop(ev->via.node);

    if (ev->tp == TI_EVENT_TP_EPKG)
        ti_epkg_drop(ev->via.epkg);

    vec_destroy(ev->_tasks, (vec_destroy_cb) ti_task_destroy);

    free(ev);
}

/* (Log function)
 * Returns 0 when successful, or -1 in case of an error.
 * This function creates the event tasks.
 *
 *  { [0, 0]: {0: [ {'job':...} ] } }
 */
int ti_event_run(ti_event_t * ev)
{
    assert (ev->tp == TI_EVENT_TP_EPKG);
    assert (ev->via.epkg);
    assert (ev->via.epkg->event_id == ev->id);
    assert (ev->via.epkg->pkg->tp == TI_PROTO_NODE_EVENT ||
            ev->via.epkg->pkg->tp == TI_PROTO_NODE_REQ_SYNCEPART);

    int rc = -1;
    ti_pkg_t * pkg = ev->via.epkg->pkg;
    qp_unpacker_t unpacker;
    qp_obj_t qp_scope, thing_or_map;
    uint64_t scope_id;

    qp_unpacker_init(&unpacker, pkg->data, pkg->n);

    if (    !qp_is_map(qp_next(&unpacker, NULL)) ||
            !qp_is_array(qp_next(&unpacker, NULL)) ||       /* fixed size 2 */
            !qp_is_int(qp_next(&unpacker, NULL)) ||         /* event_id     */
            !qp_is_int(qp_next(&unpacker, &qp_scope)) ||    /* scope        */
            !qp_is_map(qp_next(&unpacker, NULL)))           /* map with
                                                               thing_id:task */
    {
        log_critical("invalid or corrupt: "TI_EVENT_ID, ev->id);
        return rc;
    }

    scope_id = (uint64_t) qp_scope.via.int64;
    if (!scope_id)
        ev->collection = NULL;      /* scope 0 (TI_SCOPE_THINGSDB) is root */
    else
    {
        ev->collection = ti_collections_get_by_id(scope_id);
        if (!ev->collection)
        {
            log_critical(
                    "target "TI_COLLECTION_ID" for "TI_EVENT_ID" not found",
                    scope_id, ev->id);
            return rc;
        }
        ti_incref(ev->collection);
    }

    qp_next(&unpacker, &thing_or_map);

    ti_events_keep_dropped();

    while (qp_is_int(thing_or_map.tp) && qp_is_array(qp_next(&unpacker, NULL)))
    {
        ti_thing_t * thing;
        uint64_t thing_id = (uint64_t) thing_or_map.via.int64;

        thing = ev->collection == NULL
                ? ti()->thing0
                : ti_collection_thing_by_id(ev->collection, thing_id);

        if (!thing)
        {
            /* can only happen if we have a collection */
            assert (ev->collection);
            log_critical(
                    "thing "TI_THING_ID" not found in collection `%.*s`, "
                    "skip "TI_EVENT_ID,
                    thing_id,
                    (int) ev->collection->name->n,
                    (const char *) ev->collection->name->data,
                    ev->id);
            goto failed;
        }

        if (ev->collection)
        {
            while (qp_is_map(qp_next(&unpacker, &thing_or_map)))
                if (ti_job_run(ev->collection, thing, &unpacker))
                {
                    log_critical(
                            "job for thing "TI_THING_ID" in "
                            TI_EVENT_ID" for collection `%.*s` failed",
                            thing_id, ev->id,
                            (int) ev->collection->name->n,
                            (const char *) ev->collection->name->data);
                    goto failed;
                }
        }
        else
        {
            while (qp_is_map(qp_next(&unpacker, &thing_or_map)))
                if (ti_rjob_run(ev, &unpacker))
                {
                    log_critical(
                            "job for `root` in "TI_EVENT_ID" failed",
                            ev->id);
                    goto failed;
                }
        }

        if (qp_is_close(thing_or_map.tp))
            qp_next(&unpacker, &thing_or_map);
    }

    rc = 0;

failed:
    ti_events_free_dropped();

    return rc;
}

void ti__event_log_(const char * prefix, ti_event_t * ev, int log_level)
{
    log_with_level(log_level, "%s, event details:", prefix);

    uv_mutex_lock(&Logger.lock);
    (void) fprintf(Logger.ostream, "\t"TI_EVENT_ID" (", ev->id);

    switch ((ti_event_tp_enum) ev->tp)
    {
    case TI_EVENT_TP_MASTER:
        (void) fprintf(Logger.ostream, "task count: %"PRIu32, ev->_tasks->n);
        break;
    case TI_EVENT_TP_SLAVE:
        (void) fprintf(Logger.ostream, "status: %s", ti_event_status_str(ev));
        break;
    case TI_EVENT_TP_EPKG:
        qp_fprint(
                Logger.ostream,
                ev->via.epkg->pkg->data,
                ev->via.epkg->pkg->n);
        break;
    }

    (void) fprintf(Logger.ostream, ")\n");
    uv_mutex_unlock(&Logger.lock);
}

const char * ti_event_status_str(ti_event_t * ev)
{
    switch ((ti_event_tp_enum) ev->status)
    {
    case TI_EVENT_STAT_NEW:         return "NEW";
    case TI_EVENT_STAT_CACNCEL:     return "CANCEL";
    case TI_EVENT_STAT_READY:       return "READY";
    }
    return "UNKNOWN";
}
