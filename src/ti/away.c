/*
 * ti/away.c
 */
#include <assert.h>
#include <ti/away.h>
#include <ti/proto.h>
#include <ti/quorum.h>
#include <ti/things.h>
#include <ti/syncer.h>
#include <ti/syncfull.h>
#include <ti/syncarchive.h>
#include <ti/syncevents.h>
#include <ti.h>
#include <util/logger.h>
#include <util/util.h>
#include <uv.h>

static ti_away_t * away = NULL;
static uv_work_t away__uv_work;
static uv_timer_t away__uv_trigger;
static uv_timer_t away__uv_waiter;

#define AWAY__RESET_COUNTER 5   /* reset expect after X times reject   */
#define AWAY__SOON_TIMER 10000  /* in soon mode for X seconds  */
#define AWAY__BLOCK_TIME 15000  /* block accepting for X seconds */

enum away__status
{
    /* The status shifts from INIT to ACCEPTED_INIT, and from IDLE
     * to ACCEPTED_IDLE, so they must lay 2 steps from apart.
     */
    AWAY__STATUS_INIT,
    AWAY__STATUS_IDLE,
    AWAY__STATUS_ACCEPTED_INIT,
    AWAY__STATUS_ACCEPTED_IDLE,
    AWAY__STATUS_REQ_AWAY,
    AWAY__STATUS_WAITING,
    AWAY__STATUS_WORKING,
    AWAY__STATUS_SYNCING,
};

static _Bool away__required(void)
{
    return (
        ti()->archive->queue->n ||
        ti_nodes_require_sync() ||
        ti_backups_require_away()
    );
}

static const char * away__status_str(void)
{
    switch ((enum away__status) away->status)
    {
    case AWAY__STATUS_INIT:             return "INIT";
    case AWAY__STATUS_IDLE:             return "IDLE";
    case AWAY__STATUS_ACCEPTED_INIT:    return "ACCEPTED_NODE";
    case AWAY__STATUS_ACCEPTED_IDLE:    return "ACCEPTED_NODE";
    case AWAY__STATUS_REQ_AWAY:         return "REQ_AWAY";
    case AWAY__STATUS_WAITING:          return "WAITING";
    case AWAY__STATUS_WORKING:          return "WORKING";
    case AWAY__STATUS_SYNCING:          return "SYNCING";
    }
    return "UNKNOWN";
}

static void away__destroy(void)
{
    if (away)
    {
        vec_destroy(away->syncers, (vec_destroy_cb) ti_watch_drop);
        free(away);
    }
    away = ti()->away = NULL;
}

static void away__update_sleep(void)
{
    vec_t * nodes_vec = ti()->nodes->vec;
    ti_node_t * this_node = ti()->node;
    size_t idx = 0, my_idx = 0, c_idx = 0, n = nodes_vec->n;

    for (vec_each(nodes_vec, ti_node_t, node), ++idx)
    {
        if (node->id == away->away_node_id)
            c_idx = idx;
        if (node == this_node)
            my_idx = idx;
    }

    /* get the next `READY` node,  *after* the current away node */
    do
    {
        ++c_idx;
        c_idx %= n;
    }
    while (
        c_idx != my_idx &&
        ((ti_node_t *) vec_get(nodes_vec, c_idx))->status != TI_NODE_STAT_READY
    );

    if (c_idx > my_idx)
        my_idx += n;

    /* update expected node id */
    away->sleep = 2500 + ((my_idx - c_idx) * 17000);
}

static void away__accept_close_cb(uv_handle_t * UNUSED(handle))
{
    assert (away->status == AWAY__STATUS_ACCEPTED_IDLE ||
            away->status == AWAY__STATUS_ACCEPTED_INIT);
    away->status -= 2;
}

static void away__accept_cb(uv_timer_t * waiter)
{
    (void) uv_timer_stop(waiter);
    uv_close((uv_handle_t *) waiter, away__accept_close_cb);
}

static void away__accept_node_id(uint32_t node_id)
{
    int rc;
    assert (away->status == AWAY__STATUS_IDLE ||
            away->status == AWAY__STATUS_INIT);
    away->status += 2;

    ti_away_set_away_node_id(node_id);

    rc = uv_timer_init(ti()->loop, &away__uv_waiter);
    if (rc)
        return;

    rc = uv_timer_start(
            &away__uv_waiter,
            away__accept_cb,
            AWAY__BLOCK_TIME,
            0);
}

static void away__work(uv_work_t * UNUSED(work))
{
    uv_mutex_lock(ti()->events->lock);

    if ((ti()->flags & TI_FLAG_NODES_CHANGED) && ti_save() == 0)
        ti()->flags &= ~TI_FLAG_NODES_CHANGED;

    /* garbage collect */
    (void) ti_collections_gc();

    /* shrinks dropped to a reasonable size */
    (void) ti_events_resize_dropped();

    if (ti_archive_to_disk())
        log_critical("failed writing archived events to disk");

    /* write global status to disk */
    (void) ti_nodes_write_global_status();

    /* backup ThingsDB if backups are pending */
    (void) ti_backups_backup();

    uv_mutex_unlock(ti()->events->lock);
}

static size_t away__syncers(void)
{
    size_t count = 0;
    uint64_t fa_event_id = ti_archive_get_first_event_id();
    uint64_t fs_event_id = ti()->store->last_stored_event_id;

    for (vec_each(away->syncers, ti_syncer_t, syncer))
    {
        if (syncer->stream)
        {
            int rc;

            ++count;
            if (syncer->stream->flags & TI_STREAM_FLAG_SYNCHRONIZING)
                continue;

            log_info(
                    "start synchronizing `%s`",
                    ti_stream_name(syncer->stream));

            syncer->stream->flags |= TI_STREAM_FLAG_SYNCHRONIZING;

            if (    (syncer->first < fa_event_id) &&
                    syncer->first <= fs_event_id)
            {
                log_info(
                    "full database sync is required for `%s` because the "
                    "requested "TI_EVENT_ID" is not in the archive starting "
                    "at "TI_EVENT_ID" but within the full stored "TI_EVENT_ID,
                    ti_stream_name(syncer->stream),
                    syncer->first,
                    fa_event_id == UINT64_MAX ? fs_event_id + 1 : fa_event_id,
                    fs_event_id);
                if (ti_syncfull_start(syncer->stream))
                    log_critical(EX_MEMORY_S);
                continue;
            }

            rc = ti_syncarchive_init(syncer->stream, syncer->first);
            if (rc > 0)
            {
                rc = ti_syncevents_init(syncer->stream, syncer->first);
                if (rc > 0)
                {
                    rc = ti_syncevents_done(syncer->stream);
                }
            }
            if (rc < 0)
            {
                log_critical(EX_MEMORY_S);
            }
        }
    }
    return count;
}

static void away__waiter_after_close_cb(uv_handle_t * UNUSED(handle))
{
    away->status = AWAY__STATUS_IDLE;
    ti_set_and_broadcast_node_status(TI_NODE_STAT_READY);
}

static void away__waiter_after_cb(uv_timer_t * waiter)
{
    assert (away->status == AWAY__STATUS_SYNCING);

    size_t nsyncers;
    ssize_t events_to_process = ti_events_trigger_loop();

    /*
     * First check and process events before start with synchronizing
     * optional nodes. This order is required so nodes will receive events
     * from the archive queue which they might require.
     */
    if (events_to_process)
    {
        log_warning(
                "stay in away mode since the queue contains %zd %s",
                events_to_process,
                events_to_process == 1 ? "event" : "events");
        return;
    }

    nsyncers = away__syncers();
    if (nsyncers)
    {
        log_warning(
                "stay in away mode since this node is synchronizing with "
                "%zu other %s",
                nsyncers,
                nsyncers == 1 ? "node" : "nodes");
        return;
    }

    (void) uv_timer_stop(waiter);
    uv_close((uv_handle_t *) waiter, away__waiter_after_close_cb);
}

static void away__work_finish(uv_work_t * UNUSED(work), int status)
{
    away->status = AWAY__STATUS_SYNCING;

    int rc;
    if (status)
        log_error(uv_strerror(status));

    rc = uv_timer_init(ti()->loop, &away__uv_waiter);
    if (rc)
        goto fail1;

    rc = uv_timer_start(
            &away__uv_waiter,
            away__waiter_after_cb,
            0,          /* check immediately, no reason to wait */
            2000        /* check on repeat if finished */
    );

    if (rc)
        goto fail2;

    return;

fail2:
    uv_close((uv_handle_t *) &away__uv_waiter, NULL);

fail1:
    log_error("cannot start `away` waiter: `%s`", uv_strerror(rc));
    away->status = AWAY__STATUS_IDLE;
    ti_set_and_broadcast_node_status(TI_NODE_STAT_READY);
}

static void away__waiter_pre_close_cb(uv_handle_t * UNUSED(handle))
{
    away->status = AWAY__STATUS_WORKING;
    if (uv_queue_work(
            ti()->loop,
            &away__uv_work,
            away__work,
            away__work_finish))
    {
        away->status = AWAY__STATUS_IDLE;
        ti_set_and_broadcast_node_status(TI_NODE_STAT_READY);
        return;
    }

    ti_set_and_broadcast_node_status(TI_NODE_STAT_AWAY);
}

static void away__waiter_pre_cb(uv_timer_t * waiter)
{
    ssize_t events_to_process = ti_events_trigger_loop();
    if (events_to_process)
    {
        /* empty the queue because other nodes might wait for these evens to
         * be processed
         */
        log_warning(
                "waiting for %zd %s to finish before going to away mode",
                events_to_process,
                events_to_process == 1 ? "event" : "events");

        return;
    }

    if (ti()->flags & TI_FLAG_SIGNAL)
        return;

    (void) uv_timer_stop(waiter);
    uv_close((uv_handle_t *) waiter, away__waiter_pre_close_cb);
}

static void away__reset_sleep(void)
{
    vec_t * nodes_vec = ti()->nodes->vec;
    away->away_node_id = ((ti_node_t *) vec_first(nodes_vec))->id;
    away__update_sleep();
}

static void away__on_req_away_id(void * UNUSED(data), _Bool accepted)
{
    if (!accepted)
    {
        log_info(
            "not received the required quorum (%u) for going into away mode",
            ti_nodes_quorum());
        goto fail0;;
    }

    if (uv_timer_init(ti()->loop, &away__uv_waiter))
        goto fail1;

    if (uv_timer_start(
            &away__uv_waiter,
            away__waiter_pre_cb,
            AWAY__SOON_TIMER,   /* x seconds we keep in AWAY_SOON mode */
            1000                /* a little longer if events are still queued */
    ))
        goto fail2;

    ti_away_set_away_node_id(ti()->node->id);
    ti_set_and_broadcast_node_status(TI_NODE_STAT_AWAY_SOON);
    away->status = AWAY__STATUS_WAITING;
    return;

fail2:
    uv_close((uv_handle_t *) &away__uv_waiter, NULL);
fail1:
    log_critical(EX_INTERNAL_S);
fail0:
    away->status = AWAY__STATUS_IDLE;
}

static void away__req_away_id(void)
{
    vec_t * nodes_vec = ti()->nodes->vec;
    ti_quorum_t * quorum = NULL;
    ti_pkg_t * pkg, * dup;

    quorum = ti_quorum_new((ti_quorum_cb) away__on_req_away_id, NULL);
    if (!quorum)
        goto failed;

    pkg = ti_pkg_new(0, TI_PROTO_NODE_REQ_AWAY, NULL, 0);
    if (!pkg)
        goto failed;

    for (vec_each(nodes_vec, ti_node_t, node))
    {
        if (node == ti()->node)
            continue;

        dup = NULL;
        if (node->status <= TI_NODE_STAT_SHUTTING_DOWN ||
            !(dup = ti_pkg_dup(pkg)) ||
            ti_req_create(
                node->stream,
                dup,
                TI_PROTO_NODE_REQ_AWAY_ID_TIMEOUT,
                ti_quorum_req_cb,
                quorum))
        {
            free(dup);
            if (ti_quorum_shrink_one(quorum))
                log_error(
                    "failed to reach quorum of %u nodes while the previous "
                    "check was successful", quorum->quorum);
        }
    }

    free(pkg);
    ti_quorum_go(quorum);

    return;

failed:
    ti_quorum_destroy(quorum);
    log_critical(EX_MEMORY_S);
    away->status = AWAY__STATUS_IDLE;
}

static void away__trigger_cb(uv_timer_t * UNUSED(repeat))
{
    static const char * away__skip_msg = "not going in away mode (%s)";
    ti_node_t * node;

    if (ti()->nodes->vec->n == 1)
    {
        log_debug(away__skip_msg, "running as single node");
        return;
    }

    switch (away->status)
    {
    case AWAY__STATUS_INIT:
        log_debug(away__skip_msg, "away mode is not initialized");
        return;
    case AWAY__STATUS_IDLE:
        break;
    case AWAY__STATUS_ACCEPTED_INIT:
    case AWAY__STATUS_ACCEPTED_IDLE:
        log_debug(away__skip_msg, "accepted another node to go into away mode");
        return;
    case AWAY__STATUS_REQ_AWAY:
    case AWAY__STATUS_WAITING:
    case AWAY__STATUS_WORKING:
    case AWAY__STATUS_SYNCING:
        return;  /* this node is going into away mode, no logging */
    }

    if (ti()->node->status != TI_NODE_STAT_READY)
    {
        log_debug(away__skip_msg, "node status is not ready");
        return;
    }

    if (!away__required())
    {
        log_debug(away__skip_msg, "no reason for going into away mode");
        return;
    }

    if ((node = ti_nodes_get_away_or_soon()))
    {
        log_debug(away__skip_msg, "other node is away or going away soon");
        ti_away_set_away_node_id(node->id);
        return;
    }

    if (!ti_nodes_has_quorum())
    {
        log_debug(away__skip_msg, "quorum not reached");
        return;
    }

    away->status = AWAY__STATUS_REQ_AWAY;
    away__req_away_id();
}

int ti_away_create(void)
{
    away = malloc(sizeof(ti_nodes_t));
    if (!away)
        return -1;

    away->syncers = vec_new(1);
    away->status = AWAY__STATUS_INIT;
    away->away_node_id = 0;
    away->sleep = 0;

    if (!away->syncers)
    {
        away__destroy();
        return -1;
    }

    ti()->away = away;
    return 0;
}

int ti_away_start(void)
{
    assert (away->status == AWAY__STATUS_INIT);

    away__reset_sleep();

    if (uv_timer_init(ti()->loop, &away__uv_trigger))
        goto fail0;

    if (uv_timer_start(
            &away__uv_trigger,
            away__trigger_cb,
            away->sleep,
            away->sleep))
        goto fail1;

    away->status = AWAY__STATUS_IDLE;
    return 0;

fail1:
    uv_close((uv_handle_t *) &away__uv_trigger, NULL);
    away__destroy();
fail0:
    return -1;
}

void ti_away_stop(void)
{
    if (!away)
        return;

    if (away->status == AWAY__STATUS_ACCEPTED_INIT ||
        away->status == AWAY__STATUS_ACCEPTED_IDLE ||
        away->status == AWAY__STATUS_WAITING ||
        away->status == AWAY__STATUS_SYNCING)
    {
        if (!uv_is_closing((uv_handle_t *) &away__uv_waiter))
        {
            uv_timer_stop(&away__uv_waiter);
            uv_close((uv_handle_t *) &away__uv_waiter, NULL);
        }
    }

    if (away->status != AWAY__STATUS_INIT)
    {
        uv_timer_stop(&away__uv_trigger);
        uv_close((uv_handle_t *) &away__uv_trigger, NULL);
    }

    away__destroy();
}

_Bool ti_away_accept(uint32_t node_id)
{
    ti_node_t * node = NULL;

    switch ((enum away__status) away->status)
    {
    case AWAY__STATUS_INIT:
    case AWAY__STATUS_IDLE:
        away__accept_node_id(node_id);
        return true;
    case AWAY__STATUS_ACCEPTED_INIT:
    case AWAY__STATUS_ACCEPTED_IDLE:
        node = ti_nodes_get_away_or_soon();
        break;
    case AWAY__STATUS_REQ_AWAY:
    case AWAY__STATUS_WAITING:
    case AWAY__STATUS_WORKING:
    case AWAY__STATUS_SYNCING:
        node = ti()->node;
        break;
    }

    if (node)
        ti_away_set_away_node_id(node->id);

    if (away->away_node_id == node_id)
        return true;

    log_warning(
            "reject away request from "TI_NODE_ID
            "; away status: `%s`; (pending) away node: "TI_NODE_ID,
            node_id, away__status_str(), away->away_node_id);

    return false;
}

void ti_away_set_away_node_id(uint32_t node_id)
{
    if (node_id == away->away_node_id)
        return;
    away->away_node_id = node_id;
    away__update_sleep();
    uv_timer_set_repeat(&away__uv_trigger, away->sleep);
}

void ti_away_reschedule(void)
{
    uint32_t prev_sleep = away->sleep;
    away__update_sleep();
    if (prev_sleep != away->sleep)
        uv_timer_set_repeat(&away__uv_trigger, away->sleep);
}

_Bool ti_away_is_working(void)
{
    return away->status == AWAY__STATUS_WORKING;
}

int ti_away_syncer(ti_stream_t * stream, uint64_t first)
{
    ti_syncer_t * syncer;
    ti_syncer_t ** empty_syncer = NULL;

    for (vec_each_addr(away->syncers, ti_syncer_t, syncr))
    {
        if ((*syncr)->stream == stream)
        {
            (*syncr)->first = first;
            return 0;
        }
        if (!(*syncr)->stream)
        {
            empty_syncer = syncr;
        }
    }

    if (empty_syncer)
    {
        syncer = *empty_syncer;
        syncer->stream = stream;
        syncer->first = first;
        goto finish;
    }

    syncer = ti_syncer_create(stream, first);
    if (!syncer)
        return -1;

    if (vec_push(&away->syncers, syncer))
        goto failed;

finish:
    if (!stream->watching)
    {
        stream->watching = vec_new(1);
        if (!stream->watching)
            goto failed;
        VEC_push(stream->watching, syncer);
    }
    else if (vec_push(&stream->watching, syncer))
        goto failed;

    return 0;

failed:
    /* when this fails, a few bytes might leak */
    syncer->stream = NULL;
    return -1;
}

void ti_away_syncer_done(ti_stream_t * stream)
{
    size_t i = 0;
    for (vec_each(away->syncers, ti_syncer_t, syncer), ++i)
    {
        if (syncer->stream == stream)
        {
            /* remove the synchronizing flag */
            syncer->stream->flags &= ~TI_STREAM_FLAG_SYNCHRONIZING;
            break;
        }
    }

    if (i < away->syncers->n)
    {
        ti_watch_drop((ti_watch_t *) vec_swap_remove(away->syncers, i));
    }
}
