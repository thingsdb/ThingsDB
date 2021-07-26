/*
 * ti/changes.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/change.h>
#include <ti/changes.h>
#include <ti.h>
#include <ti/cpkg.h>
#include <ti/cpkg.inline.h>
#include <ti/proto.h>
#include <ti/query.h>
#include <ti/query.inline.h>
#include <ti/quorum.h>
#include <ti/thing.h>
#include <util/fx.h>
#include <util/logger.h>
#include <util/mpack.h>
#include <util/util.h>
#include <util/vec.h>

/*
 * When waiting for a missing change for this amount of time, continue and
 * accept that the missing change is not received;
 */
#define CHANGES__TIMEOUT 42.0

/*
 * When a new change is created and no quorum is reach in this amount of time,
 * kill the change and continue;
 */
#define CHANGES__NEW_TIMEOUT 21.0

/*
 * When waiting for a missing change for this amount of time, do an attempt to
 * request the missing change to a random online node;
 */
#define CHANGES__MISSING_TIMEOUT 6.0

/*
 * Avoid extreme gaps between change id's
 */
#define CHANGES__MAX_ID_GAP 1000

/*
 * Initial dropped queue size
 */
#define CHANGES__INIT_DROPPED_SZ 128

ti_changes_t changes_;
static ti_changes_t * changes;

static void changes__destroy(uv_handle_t * UNUSED(handle));
static void changes__new_id(ti_change_t * change);
static int changes__req_change_id(ti_change_t * change, ex_t * e);
static void changes__on_req_change_id(ti_change_t * change, _Bool accepted);
static int changes__push(ti_change_t * change);
static void changes__loop(uv_async_t * handle);

static inline ssize_t changes__trigger(void)
{
    return changes->queue->n
            ? (
                    uv_async_send(changes->changeloop)
                        ? -1                          /* error */
                        : (ssize_t) changes->queue->n  /* current size */
            )
            : 0;
}

static inline _Bool changes__max_id_gap(uint64_t change_id)
{
    return (
        change_id > changes->next_change_id &&
        change_id - changes->next_change_id > CHANGES__MAX_ID_GAP
    );
}

/*
 * Returns 0 on success
 * - creates singleton `changes``
 */
int ti_changes_create(void)
{
    changes = &changes_;

    changes->is_started = false;
    changes->keep_dropped = false;
    changes->queue = queue_new(4);
    changes->changeloop = malloc(sizeof(uv_async_t));
    changes->lock = malloc(sizeof(uv_mutex_t));
    changes->next_change_id = 0;
    changes->dropped = vec_new(CHANGES__INIT_DROPPED_SZ);
    changes->skipped_ids = olist_create();
    memset(&changes->wait_gap_time, 0, sizeof(changes->wait_gap_time));
    changes->wait_ccid = 0;

    if (!changes->skipped_ids ||
        !changes->lock ||
        uv_mutex_init(changes->lock))
    {
        log_critical("failed to initiate uv_mutex lock");
        goto failed;
    }

    if (!changes->queue || !changes->changeloop)
        goto failed;

    ti.changes = changes;
    return 0;

failed:
    ti_changes_stop();
    return -1;
}

/*
 * Returns 0 on success
 * - initialize and start `changes`
 */
int ti_changes_start(void)
{
    if (uv_async_init(ti.loop, changes->changeloop, changes__loop))
        return -1;
    changes->is_started = true;
    return 0;
}

/*
 * Stop and destroy `changes`
 */
void ti_changes_stop(void)
{
    if (!changes)
        return;

    if (changes->is_started)
        uv_close((uv_handle_t *) changes->changeloop, changes__destroy);
    else
        changes__destroy(NULL);
}

/* Returns a negative number in case of an error, or else the queue size at
 * the time the trigger is started.
 */
ssize_t ti_changes_trigger_loop(void)
{
    return changes__trigger();
}

int ti_changes_on_change(ti_node_t * from_node, ti_pkg_t * pkg)
{
    int rc;
    ti_cpkg_t * cpkg = ti_cpkg_from_pkg(pkg);
    if (!cpkg)
        return -1;

    rc = ti_changes_add_change(from_node, cpkg);

    ti_cpkg_drop(cpkg);

    return rc;
}

int ti_changes_create_new_change(ti_query_t * query, ex_t * e)
{
    ti_change_t * change;

    if (!ti_nodes_has_quorum())
    {
        uint8_t quorum = ti_nodes_quorum();
        ex_set(e, EX_NODE_ERROR,
                TI_NODE_ID" does not have the required quorum "
                "of at least %u connected %s",
                ti.node->id,
                quorum, quorum == 1 ? "node" : "nodes");
        return e->nr;
    }

    if (queue_reserve(&changes->queue, 1))
    {
        ex_set_mem(e);
        return e->nr;
    }

    change = ti_change_create(TI_CHANGE_TP_MASTER);
    if (!change)
    {
        ex_set_mem(e);
        return e->nr;
    }

    change->via.query = query;
    query->change = ti_grab(change);
    change->collection = ti_grab(query->collection);

    return changes__req_change_id(change, e);
}

/*
 * Returns 0 on success, < 0 when critical, > 0 if not added for a reason
 * - function performs logging on failure
 * - `node` is required but only used for logging
 * - increments a reference for `cpkg` on success
 */
int ti_changes_add_change(ti_node_t * node, ti_cpkg_t * cpkg)
{
    ti_change_t * change = NULL;

    if (changes__max_id_gap(cpkg->change_id))
    {
        log_error(
                TI_CHANGE_ID" is too high compared to "
                "the next expected "TI_CHANGE_ID,
                cpkg->change_id,
                changes->next_change_id);
        return 1;
    }

    if (cpkg->change_id <= ti.node->ccid)
    {
        log_warning(TI_CHANGE_ID" is already committed", cpkg->change_id);
        return 1;
    }

    /*
     * Update the change id in an early stage, this should be done even if
     * something else fails
     */
    if (cpkg->change_id >= changes->next_change_id)
        changes->next_change_id = cpkg->change_id + 1;

    for (queue_each(changes->queue, ti_change_t, change_))
        if (change_->id == cpkg->change_id)
            change = change_;

    if (change)
    {
        if (change->status == TI_CHANGE_STAT_READY)
        {
            switch ((ti_change_tp_enum) change->tp)
            {
            case TI_CHANGE_TP_MASTER:
                log_warning(
                     TI_CHANGE_ID" is being processed and "
                     "can not be reused for "TI_NODE_ID,
                     change->id, node->id);
                 break;
            case TI_CHANGE_TP_CPKG:
                change->via.cpkg->flags |= cpkg->flags;
                break;
            }
            return 1;
        }

        assert (change->tp != TI_CHANGE_TP_CPKG);

        /* change is owned by MASTER and needs to stay with MASTER */
        change = queue_rmval(changes->queue, change);
        if (change)
            ti_change_drop(change);

        /* bubble down and create a new change */
    }

    if (queue_reserve(&changes->queue, 1))
        return -1;

    change = ti_change_create(TI_CHANGE_TP_CPKG);
    if (!change)
        return -1;

    change->via.cpkg = ti_grab(cpkg);
    change->id = cpkg->change_id;

    /* we have space so this function always succeeds */
    (void) changes__push(change);

    change->status = TI_CHANGE_STAT_READY;

    if (changes__trigger() < 0)
        log_error("cannot trigger the change loop");

    return 0;
}

/* Returns true if the change is accepted, false if not. In case the change
 * is not accepted due to an error, logging is done.
 */
ti_proto_enum_t ti_changes_accept_id(uint64_t change_id, uint8_t * n)
{
    uint64_t * ccid_p;
    olist_iter_t iter;

    if (change_id == changes->next_change_id)
    {
        ++changes->next_change_id;
        return TI_PROTO_NODE_RES_ACCEPT;
    }

    if (change_id > changes->next_change_id)
    {
        log_info("skipped %u change id%s while accepting "TI_CHANGE_ID,
                change_id - changes->next_change_id,
                change_id - changes->next_change_id == 1 ? "" : "s",
                change_id);
        do
        {
            if (olist_set(changes->skipped_ids, changes->next_change_id))
                log_error(EX_MEMORY_S);

        } while (++changes->next_change_id < change_id);

        ++changes->next_change_id;
        return TI_PROTO_NODE_RES_ACCEPT;
    }

    for (queue_each(changes->queue, ti_change_t, change))
    {
        if (change->id > change_id)
            break;

        if (change->id == change_id)
            return change->tp == TI_CHANGE_TP_MASTER && (
                        (*n = change->requests) || 1)
                    ? TI_PROTO_NODE_ERR_COLLISION
                    : TI_PROTO_NODE_ERR_REJECT;
    }

    ccid_p = &ti.node->ccid;
    iter = olist_iter(changes->skipped_ids);

    for (size_t n = changes->skipped_ids->n; n--;)
    {
        uint64_t id = olist_iter_id(iter);
        iter = iter->next_;

        if (id <= *ccid_p)
        {
            olist_rm(changes->skipped_ids, id);
            continue;
        }

        if (id == change_id)
        {
            olist_rm(changes->skipped_ids, id);
            return TI_PROTO_NODE_RES_ACCEPT;
        }

        if (id > change_id)
            break;
    }

    return TI_PROTO_NODE_ERR_REJECT;
}

/* Sets the next missing change id, at least higher than the given change_id.
 *
 */
void ti_changes_set_next_missing_id(uint64_t * change_id)
{
    uint64_t ccid = ti.node->ccid;

    if (ccid > *change_id)
        *change_id = ccid;

    for (queue_each(changes->queue, ti_change_t, change))
    {
        if (change->id <= *change_id)
            continue;

        if (change->id == ++(*change_id))
            continue;

        return;
    }

    ++(*change_id);
}

void ti_changes_free_dropped(void)
{
    ti_thing_t * thing;

    changes->keep_dropped = false;

    while ((thing = vec_pop(changes->dropped)))
        if (!--thing->ref)
            ti_thing_destroy(thing);

    assert (changes->dropped->n == 0);
}

/* Only call while not is use, so `n` must be zero */
int ti_changes_resize_dropped(void)
{
    assert (changes->dropped->n == 0);
    return changes->dropped->sz == CHANGES__INIT_DROPPED_SZ
            ? 0
            : vec_resize(&changes->dropped, CHANGES__INIT_DROPPED_SZ);
}

static void changes__destroy(uv_handle_t * UNUSED(handle))
{
    if (!changes)
        return;
    queue_destroy(changes->queue, (queue_destroy_cb) ti_change_drop);
    uv_mutex_destroy(changes->lock);
    free(changes->lock);
    free(changes->changeloop);
    vec_destroy(changes->dropped, (vec_destroy_cb) ti_thing_destroy);
    olist_destroy(changes->skipped_ids);
    changes = ti.changes = NULL;
}

static void changes__new_id(ti_change_t * change)
{
    ex_t e = {0};

    /* remove the change from the queue (if still there) and reserve one
     * space if nothing is removed */
    if (!queue_rmval(changes->queue, change))
    {
        ti_incref(change);
        if (queue_reserve(&changes->queue, 1))
        {
            ex_set_mem(&e);
            goto fail;
        }
    }

    if (!ti_nodes_has_quorum())
    {
        ex_set(&e, EX_NODE_ERROR,
                TI_NODE_ID" does not have the required quorum "
                "of at least %u connected nodes",
                ti.node->id,
                ti_nodes_quorum());
        goto fail;
    }

    if (changes__req_change_id(change, &e) == 0)
        return;

    /* in case of an error, `change->id` is not changed */
fail:
    ti_change_drop(change);  /* reference for the queue */
    ti_query_response(change->via.query, &e);
    change->status = TI_CHANGE_STAT_CACNCEL;
}

static int changes__req_change_id(ti_change_t * change, ex_t * e)
{
    assert (queue_space(changes->queue) > 0);

    msgpack_packer pk;
    msgpack_sbuffer buffer;
    vec_t * nodes_vec = ti.nodes->vec;
    ti_quorum_t * quorum;
    ti_pkg_t * pkg, * dup;

    quorum = ti_quorum_new((ti_quorum_cb) changes__on_req_change_id, change);
    if (!quorum)
    {
        ex_set_mem(e);
        return e->nr;
    }

    if (mp_sbuffer_alloc_init(&buffer, 32, sizeof(ti_pkg_t)))
    {
        ti_quorum_destroy(quorum);
        ex_set_mem(e);
        return e->nr;
    }

    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    ti_incref(change);
    change->id = changes->next_change_id;
    ++changes->next_change_id;

    msgpack_pack_uint64(&pk, change->id);

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_NODE_REQ_CHANGE_ID, buffer.size);

    /* we have space so this function always succeeds */
    (void) changes__push(change);

    for (vec_each(nodes_vec, ti_node_t, node))
    {
        if (node == ti.node)
            continue;

        dup = NULL;
        if (node->status <= TI_NODE_STAT_SHUTTING_DOWN ||
            !(dup = ti_pkg_dup(pkg)) ||
            ti_req_create(
                node->stream,
                dup,
                TI_PROTO_NODE_REQ_CHANGE_ID_TIMEOUT,
                ti_quorum_req_cb,
                quorum))
        {
            free(dup);
            if (ti_quorum_shrink_one(quorum))
                log_error(
                        "failed to reach quorum while the previous check"
                        "was successful");
        }
    }

    change->requests = quorum->requests;

    free(pkg);

    ti_quorum_go(quorum);

    return 0;
}

static void changes__on_req_change_id(ti_change_t * change, _Bool accepted)
{
    if (!accepted)
    {
        ++ti.counters->changes_quorum_lost;

        log_debug(TI_CHANGE_ID" quorum lost :-(", change->id);

        changes__new_id(change);
        goto done;
    }

    log_debug(TI_CHANGE_ID" quorum win :-)", change->id);

    change->status = TI_CHANGE_STAT_READY;
    if (changes__trigger() < 0)
        log_error("cannot trigger the change loop");

done:
    ti_change_drop(change);
}

static int changes__push(ti_change_t * change)
{
    size_t idx = 0;
    ti_change_t * last_change = queue_last(changes->queue);

    if (!last_change || change->id > last_change->id)
        return queue_push(&changes->queue, change);

    ++ti.counters->changes_unaligned;
    for (queue_each(changes->queue, ti_change_t, change_), ++idx)
        if (change_->id > change->id)
            break;

    return queue_insert(&changes->queue, idx, change);
}

static void changes__loop(uv_async_t * UNUSED(handle))
{
    ti_change_t * change;
    util_time_t timing;
    uint64_t * ccid_p = &ti.node->ccid;
    int process_changes = 5;

    if (uv_mutex_trylock(changes->lock))
        return;

    if (clock_gettime(TI_CLOCK_MONOTONIC, &timing))
        goto stop;

    while (process_changes-- && (change = queue_first(changes->queue)))
    {
        /* Cancelled change should be removed from the queue */
        assert (change->status != TI_CHANGE_STAT_CACNCEL);

        if (change->id <= *ccid_p)
        {
            log_warning(
                TI_CHANGE_ID" will be skipped because "TI_CHANGE_ID
                " is already committed",
                change->id, *ccid_p);

            /*
             * TODO: We might create some kind of queue where to put skipped
             *       changes. The best option would probably to get the change
             *       mixed inside the archive, so the node can mark a flag
             *       and perform a re-load using the existing archive combined
             *       with the skipped changes.
             */
            ++ti.counters->changes_skipped;

            goto shift_drop_loop;
        }
        else if (change->id > (*ccid_p) + 1)
        {
            double diff;

            /* continue if the change is allowed a gap */
            if (change->tp == TI_CHANGE_TP_CPKG &&
                (change->via.cpkg->flags & TI_CPKG_FLAG_ALLOW_GAP))
                goto process;

            /* wait if the node is synchronizing */
            if (ti.node->status == TI_NODE_STAT_SYNCHRONIZING)
                break;

            if (changes->wait_ccid != *ccid_p)
            {
                changes->wait_ccid = *ccid_p;
                clock_gettime(TI_CLOCK_MONOTONIC, &changes->wait_gap_time);
                break;
            }

            diff = util_time_diff(&changes->wait_gap_time, &timing);

            if (diff > CHANGES__MISSING_TIMEOUT)
                ti_change_missing((*ccid_p) + 1);

            if (diff < CHANGES__TIMEOUT)
                break;

            ++(*ccid_p);
            ++ti.counters->changes_with_gap;

            log_warning(
                "committed "TI_CHANGE_ID" since the change is not received "
                "within approximately %f seconds", *ccid_p, diff);

            if (change->id > (*ccid_p) + 1)
                break;
        }

        if (change->status == TI_CHANGE_STAT_NEW)
        {
            double diff = util_time_diff(&change->time, &timing);
            /*
             * A change must have status READY before we can continue;
             */
            if (diff < CHANGES__NEW_TIMEOUT)
                break;

            log_error(
                    "killed "TI_CHANGE_ID" on "TI_NODE_ID
                    " after approximately %f seconds",
                    change->id,
                    ti.node->id,
                    diff);

            /* Reached time-out, kill the change */
            ++ti.counters->changes_killed;

            goto shift_drop_loop;
        }

process:
        assert (change->status == TI_CHANGE_STAT_READY);

        ti_change_log("processing", change, LOGGER_DEBUG);

        if (change->tp == TI_CHANGE_TP_MASTER)
        {
            ti_query_run(change->via.query);
        }
        else if (ti_change_run(change) || ti_archive_push(change->via.cpkg))
        {
            /* logging is done, but we increment the failed counter and
             * log the full change */
            ++ti.counters->changes_failed;
            ti_change_log("change has failed", change, LOGGER_ERROR);
        }

        /* update counters */
        (void) ti_counters_upd_commit_change(&change->time);

        /* update committed change id */
        *ccid_p = change->id;

        if (change->flags & TI_CHANGE_FLAG_SAVE)
            ti_save();

shift_drop_loop:
        (void) queue_shift(changes->queue);
        ti_change_drop(change);
    }

stop:
    uv_mutex_unlock(changes->lock);

    /* status will be send to nodes on next `connect` loop */
    ti_connect_force_sync();

    /* trigger the change loop again (only if there are changes) */
    (void) changes__trigger();
}

