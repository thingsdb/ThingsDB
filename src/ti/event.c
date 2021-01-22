/*
 * ti/event.c
 */
#include <assert.h>
#include <ex.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/collection.inline.h>
#include <ti/collections.h>
#include <ti/epkg.h>
#include <ti/epkg.inline.h>
#include <ti/event.h>
#include <ti/job.h>
#include <ti/node.h>
#include <ti/proto.h>
#include <ti/rjob.h>
#include <ti/task.h>
#include <ti/watch.h>
#include <util/logger.h>
#include <util/mpack.h>
#include <util/omap.h>


static uint64_t event__req_id = 0;

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

ti_event_t * ti_event_epkg(ti_epkg_t * epkg)
{
    ti_event_t * ev;
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

ti_event_t * ti_event_initial(void)
{
    ti_epkg_t * epkg = ti_epkg_initial();
    if (!epkg)
        return NULL;
    return ti_event_epkg(epkg);
}

void ti_event_drop(ti_event_t * ev)
{
    if (!ev || --ev->ref)
        return;

    ti_collection_drop(ev->collection);

    if (ev->tp == TI_EVENT_TP_EPKG)
        ti_epkg_drop(ev->via.epkg);

    vec_destroy(ev->_tasks, (vec_destroy_cb) ti_task_destroy);

    free(ev);
}

int ti_event_append_pkgs(ti_event_t * ev, ti_thing_t * thing, vec_t ** pkgs)
{
    ti_pkg_t * pkg = ev->via.epkg->pkg;
    mp_unp_t up;
    size_t i;
    mp_obj_t obj, mp_scope, mp_id;
    const char * jobs_position;

    mp_unp_init(&up, pkg->data, pkg->n);

    if (mp_next(&up, &obj) != MP_MAP || obj.via.sz != 1 ||
        /* key */
        mp_next(&up, &obj) != MP_ARR || obj.via.sz != 2 ||
        mp_skip(&up) != MP_U64 ||                       /* event id */
        mp_next(&up, &mp_scope) != MP_U64 ||            /* scope id */
        /* value */
        mp_next(&up, &obj) != MP_MAP)           /* map with thing_id:task */
    {
        log_critical("invalid or corrupt: "TI_EVENT_ID, ev->id);
        return -1;
    }

    if (mp_scope.via.u64 != thing->collection->root->id)
        return 0;  /* not a collection scope */

    for (i = obj.via.sz; i--;)
    {
        if (mp_next(&up, &mp_id) != MP_U64)
            goto fail_mp_data;

        /* keep the current position so we can update watchers */
        jobs_position = up.pt;

        if (mp_skip(&up) != MP_ARR)
            goto fail_mp_data;

        if (mp_id.via.u64 == thing->id)
        {
            size_t n = up.pt - jobs_position;
            ti_pkg_t * pkg = ti_watch_pkg(
                    thing->id,
                    ev->id,
                    (const unsigned char *) jobs_position, n);

            if (!pkg ||
                (*pkgs == NULL && !(*pkgs = vec_new(7))) ||
                vec_push(pkgs, pkg))
            {
                ++ti.counters->watcher_failed;
                log_critical(EX_MEMORY_S);
            }
        }
    }

    return 0;

fail_mp_data:
    log_critical("msgpack event data incorrect for "TI_EVENT_ID, ev->id);
    return -1;
}

int ti_event_watch(ti_event_t * ev)
{
    int rc = 0;
    ti_collection_t * collection;
    ti_thing_t * thing;
    ti_pkg_t * pkg = ev->via.epkg->pkg;
    mp_unp_t up;
    size_t i;
    mp_obj_t obj, mp_scope, mp_id;
    const char * jobs_position;

    ev->flags |= TI_EVENT_FLAG_WATCHED;

    mp_unp_init(&up, pkg->data, pkg->n);

    if (mp_next(&up, &obj) != MP_MAP || obj.via.sz != 1 ||
        /* key */
        mp_next(&up, &obj) != MP_ARR || obj.via.sz != 2 ||
        mp_skip(&up) != MP_U64 ||                       /* event id */
        mp_next(&up, &mp_scope) != MP_U64 ||            /* scope id */
        /* value */
        mp_next(&up, &obj) != MP_MAP)           /* map with thing_id:task */
    {
        log_critical("invalid or corrupt: "TI_EVENT_ID, ev->id);
        return -1;
    }

    if (!mp_scope.via.u64)
        return 0;  /* not a collection scope */

    collection = ti_collections_get_by_id(mp_scope.via.u64);
    if (!collection)
    {
        log_warning(
                "target "TI_COLLECTION_ID" for "TI_EVENT_ID" not found "
                "(this may occur in case the event which will create the  "
                "collection, is still in the queue and not yet processed)",
                mp_scope.via.u64, ev->id);
        return 0;
    }

    uv_mutex_lock(collection->lock);

    for (i = obj.via.sz; i--;)
    {
        if (mp_next(&up, &mp_id) != MP_U64)
            goto fail_mp_data;

        thing = ti_collection_find_thing(collection, mp_id.via.u64);

        /* keep the current position so we can update watchers */
        jobs_position = up.pt;

        if (mp_skip(&up) != MP_ARR)
            goto fail_mp_data;

        /*
         * The `thing` may be NULL here since it is possible the ID will be
         * created by this event. In this case it is not possible that there
         * are watchers so it can be skipped without problems.
         */
        if (thing && ti_thing_has_watchers(thing))
        {
            size_t n = up.pt - jobs_position;
            ti_rpkg_t * rpkg = ti_watch_rpkg(
                    thing->id,
                    ev->id,
                    (const unsigned char *) jobs_position, n);

            if (rpkg)
            {
                for (vec_each(thing->watchers, ti_watch_t, watch))
                {
                    if (ti_stream_is_closed(watch->stream))
                        continue;

                    if (ti_stream_write_rpkg(watch->stream, rpkg))
                    {
                        ++ti.counters->watcher_failed;
                        log_error(EX_INTERNAL_S);
                    }
                }
                ti_rpkg_drop(rpkg);
            }
            else
            {
                ++ti.counters->watcher_failed;
                log_critical(EX_MEMORY_S);
            }
        }
    }

    goto done;

fail_mp_data:
    log_critical("msgpack event data incorrect for "TI_EVENT_ID, ev->id);
    rc = -1;

done:
    uv_mutex_unlock(collection->lock);
    return rc;
}

void ti_event_missing_event(uint64_t event_id)
{
    ti_pkg_t * pkg;
    ti_node_t * node;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (event__req_id == event_id)
        return;

    node = ti_nodes_random_ready_node();
    if (!node)
    {
        log_warning(
                "cannot find a ready node to ask for missing"TI_EVENT_ID,
                event_id);
        return;
    }

    if (mp_sbuffer_alloc_init(&buffer, 64, sizeof(ti_pkg_t)))
        return;

    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_uint64(&pk, event_id);

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_NODE_MISSING_EVENT, buffer.size);

    if (ti_stream_write_pkg(node->stream, pkg))
        free(pkg);
    else
        event__req_id = event_id;

    log_warning(
            "request missing "TI_EVENT_ID" from "TI_NODE_ID,
            event_id, node->id);
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

    ti_thing_t * thing;
    ti_pkg_t * pkg = ev->via.epkg->pkg;
    mp_unp_t up;
    size_t i, ii;
    mp_obj_t obj, mp_scope, mp_id;
    const char * jobs_position;

    mp_unp_init(&up, pkg->data, pkg->n);

    if (mp_next(&up, &obj) != MP_MAP || obj.via.sz != 1 ||
        /* key */
        mp_next(&up, &obj) != MP_ARR || obj.via.sz != 2 ||
        mp_skip(&up) != MP_U64 ||                       /* event id */
        mp_next(&up, &mp_scope) != MP_U64 ||            /* scope id */
        /* value */
        mp_next(&up, &obj) != MP_MAP)           /* map with thing_id:task */
    {
        log_critical("invalid or corrupt: "TI_EVENT_ID, ev->id);
        return -1;
    }

    if (!mp_scope.via.u64)
        ev->collection = NULL;      /* scope 0 (TI_SCOPE_THINGSDB) is root */
    else
    {
        ev->collection = ti_collections_get_by_id(mp_scope.via.u64);
        if (!ev->collection)
        {
            log_error(
                    "target "TI_COLLECTION_ID" for "TI_EVENT_ID" not found",
                    mp_scope.via.u64, ev->id);
            return -1;
        }
        ti_incref(ev->collection);
    }

    ti_events_keep_dropped();

    for (i = obj.via.sz; i--;)
    {
        /*
         * Loop over event tasks. Each iteration is a task related to a
         * thing.
         */
        if (mp_next(&up, &mp_id) != MP_U64)
            goto fail_mp_data;

        thing = ev->collection == NULL
                ? ti.thing0
                : ti_collection_find_thing(ev->collection, mp_id.via.u64);

        if (!thing)
        {
            /* can only happen if we have a collection */
            assert (ev->collection);
            log_critical(
                    "thing "TI_THING_ID" not found in collection `%.*s`, "
                    "skip "TI_EVENT_ID,
                    mp_id.via.u64,
                    (int) ev->collection->name->n,
                    (const char *) ev->collection->name->data,
                    ev->id);
            goto fail;
        }

        /* keep the current position so we can update watchers */
        jobs_position = up.pt;

        if (mp_next(&up, &obj) != MP_ARR)
            goto fail_mp_data;

        if (ev->collection)
        {
            for (ii = obj.via.sz; ii--;)
            {
                if (ti_job_run(thing, &up, ev->id))
                {
                    log_critical(
                            "job for thing "TI_THING_ID" in "
                            TI_EVENT_ID" for collection `%.*s` failed",
                            thing->id, ev->id,
                            (int) ev->collection->name->n,
                            (const char *) ev->collection->name->data);
                    goto fail;
                }
            }

            if (ti_event_require_watch_upd(ev) && ti_thing_has_watchers(thing))
            {
                size_t n = up.pt - jobs_position;
                ti_rpkg_t * rpkg = ti_watch_rpkg(
                        thing->id,
                        ev->id,
                        (const unsigned char *) jobs_position, n);

                if (rpkg)
                {
                    for (vec_each(thing->watchers, ti_watch_t, watch))
                    {
                        if (ti_stream_is_closed(watch->stream))
                            continue;

                        if (ti_stream_write_rpkg(watch->stream, rpkg))
                        {
                            ++ti.counters->watcher_failed;
                            log_error(EX_INTERNAL_S);
                        }
                    }
                    ti_rpkg_drop(rpkg);
                }
                else
                {
                    ++ti.counters->watcher_failed;
                    log_critical(EX_MEMORY_S);
                }
            }
        }
        else
        {
            for (ii = obj.via.sz; ii--;)
            {
                if (ti_rjob_run(ev, &up))
                {
                    log_critical(
                            "job for `root` in "TI_EVENT_ID" failed", ev->id);
                    goto fail;
                }
            }
        }
    }

    ti_events_free_dropped();
    return 0;

fail_mp_data:
    log_critical("msgpack event data incorrect for "TI_EVENT_ID, ev->id);
fail:
    ti_events_free_dropped();
    return -1;
}

void ti__event_log_(const char * prefix, ti_event_t * ev, int log_level)
{
    log_with_level(log_level, "%s: "TI_EVENT_ID" details:", prefix, ev->id);

    (void) fprintf(Logger.ostream, "\t");

    switch ((ti_event_tp_enum) ev->tp)
    {
    case TI_EVENT_TP_MASTER:
        switch ((ti_query_with_enum) ev->via.query->with_tp)
        {
        case TI_QUERY_WITH_PARSERES:
            (void) fprintf(
                    Logger.ostream,
                    "<query: `%s`>",
                    ev->via.query->with.parseres->str);
            break;
        case TI_QUERY_WITH_PROCEDURE:
            (void) fprintf(
                    Logger.ostream,
                    "<procedure: `%s`>",
                    ev->via.query->with.closure->node->str);
            break;
        case TI_QUERY_WITH_FUTURE:
            (void) fprintf(
                    Logger.ostream,
                    "<future: `%s`>",
                    ev->via.query->with.future->then->node->str);
            break;
        }
        break;
    case TI_EVENT_TP_EPKG:
        mp_print(
                Logger.ostream,
                ev->via.epkg->pkg->data,
                ev->via.epkg->pkg->n);
        break;
    }

    (void) fprintf(Logger.ostream, "\n");
}

const char * ti_event_status_str(ti_event_t * ev)
{
    switch ((ti_event_status_enum) ev->status)
    {
    case TI_EVENT_STAT_NEW:         return "NEW";
    case TI_EVENT_STAT_CACNCEL:     return "CANCEL";
    case TI_EVENT_STAT_READY:       return "READY";
    }
    return "UNKNOWN";
}
