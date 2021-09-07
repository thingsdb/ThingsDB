/*
 * ti/change.c
 */
#include <assert.h>
#include <ex.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ti/change.h>
#include <ti.h>
#include <ti/collection.inline.h>
#include <ti/collections.h>
#include <ti/cpkg.h>
#include <ti/cpkg.inline.h>
#include <ti/ctask.h>
#include <ti/node.h>
#include <ti/proto.h>
#include <ti/task.h>
#include <ti/ttask.h>
#include <ti/watch.h>
#include <util/logger.h>
#include <util/mpack.h>
#include <util/omap.h>

static uint64_t change__req_id = 0;

ti_change_t * ti_change_create(ti_change_tp_enum tp)
{
    ti_change_t * change = malloc(sizeof(ti_change_t));
    if (!change)
        return NULL;

    change->ref = 1;
    change->status = TI_CHANGE_STAT_NEW;
    change->collection = NULL;
    change->tp = tp;
    change->flags = 0;
    change->_tasks = tp == TI_CHANGE_TP_MASTER ? vec_new(1) : NULL;

    if (    (tp == TI_CHANGE_TP_MASTER && !change->_tasks) ||
            clock_gettime(TI_CLOCK_MONOTONIC, &change->time))
    {
        free(change);
        return NULL;
    }

    return change;
}

ti_change_t * ti_change_cpkg(ti_cpkg_t * cpkg)
{
    ti_change_t * change;
    change = ti_change_create(TI_CHANGE_TP_CPKG);
    if (!change)
    {
        ti_cpkg_drop(cpkg);
        return NULL;
    }

    change->status = TI_CHANGE_STAT_READY;
    change->via.cpkg = cpkg;
    change->id = cpkg->change_id;

    return change;
}

ti_change_t * ti_change_initial(void)
{
    ti_cpkg_t * cpkg = ti_cpkg_initial();
    if (!cpkg)
        return NULL;
    return ti_change_cpkg(cpkg);
}

void ti_change_drop(ti_change_t * change)
{
    if (!change || --change->ref)
        return;

    ti_collection_drop(change->collection);

    if (change->tp == TI_CHANGE_TP_CPKG)
        ti_cpkg_drop(change->via.cpkg);

    vec_destroy(change->_tasks, (vec_destroy_cb) ti_task_destroy);

    free(change);
}

void ti_change_missing(uint64_t change_id)
{
    ti_pkg_t * pkg;
    ti_node_t * node;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (change__req_id == change_id)
        return;

    node = ti_nodes_random_ready_node();
    if (!node)
    {
        log_warning(
                "cannot find a ready node to ask for missing"TI_CHANGE_ID,
                change_id);
        return;
    }

    if (mp_sbuffer_alloc_init(&buffer, 64, sizeof(ti_pkg_t)))
        return;

    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_uint64(&pk, change_id);

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_NODE_MISSING_CHANGE, buffer.size);

    if (ti_stream_write_pkg(node->stream, pkg))
        free(pkg);
    else
        change__req_id = change_id;

    log_warning(
            "request missing "TI_CHANGE_ID" from "TI_NODE_ID,
            change_id, node->id);
}

/* (Log function)
 * Returns 0 when successful, or -1 in case of an error.
 * This function creates the change tasks.
 *
 *  { [0, 0]: {0: [ {'task':...} ] } }
 */
static int change__run_deprecated_v0(ti_change_t * change)
{
    ti_thing_t * thing;
    ti_pkg_t * pkg = change->via.cpkg->pkg;
    mp_unp_t up;
    size_t i, ii;
    mp_obj_t obj, mp_scope, mp_id;

    mp_unp_init(&up, pkg->data, pkg->n);

    if (mp_next(&up, &obj) != MP_MAP || obj.via.sz != 1 ||
        /* key */
        mp_next(&up, &obj) != MP_ARR || obj.via.sz != 2 ||
        mp_skip(&up) != MP_U64 ||                       /* change id */
        mp_next(&up, &mp_scope) != MP_U64 ||            /* scope id */
        /* value */
        mp_next(&up, &obj) != MP_MAP)           /* map with thing_id:task */
    {
        log_critical("invalid or corrupt: "TI_CHANGE_ID, change->id);
        return -1;
    }

    if (!mp_scope.via.u64)
        change->collection = NULL;      /* scope 0 (TI_SCOPE_THINGSDB) is root */
    else
    {
        change->collection = ti_collections_get_by_id(mp_scope.via.u64);
        if (!change->collection)
        {
            log_error(
                    "target "TI_COLLECTION_ID" for "TI_CHANGE_ID" not found",
                    mp_scope.via.u64, change->id);
            return -1;
        }
        ti_incref(change->collection);
    }

    ti_changes_keep_dropped();

    for (i = obj.via.sz; i--;)
    {
        /*
         * Loop over change tasks. Each iteration is a task related to a
         * thing.
         */
        if (mp_next(&up, &mp_id) != MP_U64)
            goto fail_mp_data;

        thing = change->collection == NULL
                ? ti.thing0
                : ti_collection_find_thing(change->collection, mp_id.via.u64);

        if (!thing)
        {
            /* can only happen if we have a collection */
            assert (change->collection);
            log_critical(
                    "thing "TI_THING_ID" not found in collection `%.*s`, "
                    "skip "TI_CHANGE_ID,
                    mp_id.via.u64,
                    (int) change->collection->name->n,
                    (const char *) change->collection->name->data,
                    change->id);
            goto fail;
        }

        if (mp_next(&up, &obj) != MP_ARR)
            goto fail_mp_data;

        if (change->collection) for (ii = obj.via.sz; ii--;)
        {
            if (ti_ctask_run(thing, &up))
            {
                log_critical(
                        "task for "TI_THING_ID" in collection `%.*s` "
                        "("TI_CHANGE_ID") failed",
                        thing->id,
                        (int) change->collection->name->n,
                        (const char *) change->collection->name->data,
                        change->id);
                goto fail;
            }
        }
        else for (ii = obj.via.sz; ii--;)
        {
            if (ti_ttask_run(change, &up))
            {
                log_critical(
                        "thingsdb task ("TI_CHANGE_ID") failed", change->id);
                goto fail;
            }
        }
    }

    ti_changes_free_dropped();
    return 0;

fail_mp_data:
    log_critical("msgpack change data incorrect for "TI_CHANGE_ID, change->id);
fail:
    ti_changes_free_dropped();
    return -1;
}

int ti_change_run(ti_change_t * change)
{
    assert (change->tp == TI_CHANGE_TP_CPKG);
    assert (change->via.cpkg);
    assert (change->via.cpkg->change_id == change->id);
    assert (change->via.cpkg->pkg->tp == TI_PROTO_NODE_CHANGE ||
            change->via.cpkg->pkg->tp == TI_PROTO_NODE_REQ_SYNCEPART);

    ti_thing_t * thing;
    ti_pkg_t * pkg = change->via.cpkg->pkg;
    mp_unp_t up;
    size_t i, ii;
    mp_obj_t obj, mp_scope, mp_id;

    mp_unp_init(&up, pkg->data, pkg->n);

    if (mp_next(&up, &obj) != MP_ARR || obj.via.sz < 2 ||
        mp_skip(&up) != MP_U64 ||                       /* change id */
        mp_next(&up, &mp_scope) != MP_U64)              /* scope id */
        return change__run_deprecated_v0(change);

    if (!mp_scope.via.u64)
        change->collection = NULL;      /* scope 0 (TI_SCOPE_THINGSDB) is root */
    else
    {
        change->collection = ti_collections_get_by_id(mp_scope.via.u64);
        if (!change->collection)
        {
            log_error(
                    "target "TI_COLLECTION_ID" for "TI_CHANGE_ID" not found",
                    mp_scope.via.u64, change->id);
            return -1;
        }
        ti_incref(change->collection);
    }

    ti_changes_keep_dropped();

    for (i = obj.via.sz-2; i--;)
    {
        /*
         * Loop over change tasks. Each iteration is a task related to a
         * thing.
         */
        if (mp_next(&up, &obj) != MP_ARR || obj.via.sz == 0 ||
            mp_next(&up, &mp_id) != MP_U64)
            goto fail_mp_data;

        thing = change->collection == NULL
                ? ti.thing0
                : ti_collection_find_thing(change->collection, mp_id.via.u64);

        if (!thing)
        {
            /* can only happen if we have a collection */
            assert (change->collection);
            log_critical(
                    "thing "TI_THING_ID" not found in collection `%.*s`, "
                    "skip "TI_CHANGE_ID,
                    mp_id.via.u64,
                    (int) change->collection->name->n,
                    (const char *) change->collection->name->data,
                    change->id);
            goto fail;
        }

        if (change->collection) for (ii = obj.via.sz-1; ii--;)
        {
            if (ti_ctask_run(thing, &up))
            {
                log_critical(
                        "task for "TI_THING_ID" in collection `%.*s` "
                        "("TI_CHANGE_ID") failed",
                        thing->id,
                        (int) change->collection->name->n,
                        (const char *) change->collection->name->data,
                        change->id);
                goto fail;
            }
        }
        else for (ii = obj.via.sz-1; ii--;)
        {
            if (ti_ttask_run(change, &up))
            {
                log_critical(
                        "thingsdb task in "TI_CHANGE_ID" failed", change->id);
                goto fail;
            }
        }
    }

    ti_changes_free_dropped();
    return 0;

fail_mp_data:
    log_critical("msgpack change data incorrect for "TI_CHANGE_ID, change->id);
fail:
    ti_changes_free_dropped();
    return -1;
}

void ti__change_log_(const char * prefix, ti_change_t * change, int log_level)
{
    log_with_level(log_level, "%s: "TI_CHANGE_ID" details:", prefix, change->id);

    (void) fprintf(Logger.ostream, "\t");

    switch ((ti_change_tp_enum) change->tp)
    {
    case TI_CHANGE_TP_MASTER:
        switch ((ti_query_with_enum) change->via.query->with_tp)
        {
        case TI_QUERY_WITH_PARSERES:
            (void) fprintf(
                    Logger.ostream,
                    "<query: `%s`>",
                    change->via.query->with.parseres->str);
            break;
        case TI_QUERY_WITH_PROCEDURE:
            (void) fprintf(
                    Logger.ostream,
                    "<procedure: `%s`>",
                    change->via.query->with.closure->node->str);
            break;
        case TI_QUERY_WITH_FUTURE:
            (void) fprintf(
                    Logger.ostream,
                    "<future: `%s`>",
                    change->via.query->with.future->then->node->str);
            break;
        case TI_QUERY_WITH_TIMER:
            (void) fprintf(
                    Logger.ostream,
                    "<timer: `%s`>",
                    change->via.query->with.timer->closure->node->str);
            break;
        }
        break;
    case TI_CHANGE_TP_CPKG:
        mp_print(
                Logger.ostream,
                change->via.cpkg->pkg->data,
                change->via.cpkg->pkg->n);
        break;
    }

    (void) fprintf(Logger.ostream, "\n");
}

const char * ti_change_status_str(ti_change_t * change)
{
    switch ((ti_change_status_enum) change->status)
    {
    case TI_CHANGE_STAT_NEW:         return "NEW";
    case TI_CHANGE_STAT_CACNCEL:     return "CANCEL";
    case TI_CHANGE_STAT_READY:       return "READY";
    }
    return "UNKNOWN";
}
