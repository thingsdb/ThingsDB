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
#include <util/qpx.h>
#include <util/util.h>
#include <uv.h>

static ti_away_t * away = NULL;

#define AWAY__ACCEPT_COUNTER 3  /* ignore `x` requests after accepting one */
#define AWAY__SOON_TIMER 10000  /* seconds might be a nice default */

enum away__status
{
    AWAY__STATUS_INIT,
    AWAY__STATUS_IDLE,
    AWAY__STATUS_REQ_AWAY,
    AWAY__STATUS_WAITING,
    AWAY__STATUS_WORKING,
    AWAY__STATUS_SYNCING,
};

static void away__destroy(uv_handle_t * handle);
static void away__req_away_id(void);
static _Bool away__required(void);
static void away__on_req_away_id(void * UNUSED(data), _Bool accepted);
static void away__waiter_pre_cb(uv_timer_t * timer);
static void away__waiter_after_cb(uv_timer_t * waiter);
static void away__work(uv_work_t * UNUSED(work));
static void away__work_finish(uv_work_t * UNUSED(work), int status);
static inline void away__repeat_cb(uv_timer_t * repeat);
static void away__change_expected_id(uint8_t node_id);
static inline uint64_t away__calc_sleep(void);

int ti_away_create(void)
{
    away = malloc(sizeof(ti_nodes_t));
    if (!away)
        return -1;

    away->work = malloc(sizeof(uv_work_t));
    away->repeat = malloc(sizeof(uv_timer_t));
    away->waiter = malloc(sizeof(uv_timer_t));
    away->syncers = vec_new(1);
    away->status = AWAY__STATUS_INIT;
    away->accept_counter = 0;
    away->expected_node_id = 0;  /* always start with node 0 */

    if (!away->work || !away->repeat || !away->waiter || !away->syncers)
    {
        away__destroy(NULL);
        return -1;
    }

    ti()->away = away;
    return 0;
}

int ti_away_start(void)
{
    assert (away->status == AWAY__STATUS_INIT);

    if (uv_timer_init(ti()->loop, away->repeat))
        goto fail0;

    if (uv_timer_start(away->repeat,
            away__repeat_cb,
            away__calc_sleep(),
            away__calc_sleep()))
        goto fail1;

    away->status = AWAY__STATUS_IDLE;
    return 0;

fail1:
    uv_close((uv_handle_t *) away->repeat, away__destroy);
fail0:
    return -1;
}

void ti_away_trigger(void)
{
    static const char * away__skip_msg = "not going in away mode (%s)";

    if (ti()->nodes->vec->n == 1)
    {
        log_debug(away__skip_msg, "running as single node");
        return;
    }

    if ((away->status != AWAY__STATUS_IDLE))
    {
        log_debug(away__skip_msg, "away status is not idle");
        return;
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

    if (ti_nodes_get_away_or_soon())
    {
        log_debug(away__skip_msg, "other node is away or going away soon");
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

void ti_away_stop(void)
{
    if (!away)
        return;

    if (away->status == AWAY__STATUS_INIT)
    {
        away__destroy(NULL);
    }
    else
    {
        if (    away->status == AWAY__STATUS_WAITING ||
                away->status ==AWAY__STATUS_SYNCING)
        {
            uv_timer_stop(away->waiter);
            uv_close((uv_handle_t *) away->waiter, (uv_close_cb) free);
        }
        else
        {
            free(away->waiter);
        }
        uv_timer_stop(away->repeat);
        uv_close((uv_handle_t *) away->repeat, away__destroy);
    }
}

_Bool ti_away_accept(uint8_t node_id)
{
    switch ((enum away__status) away->status)
    {
    case AWAY__STATUS_INIT:
    case AWAY__STATUS_IDLE:
        break;
    case AWAY__STATUS_REQ_AWAY:
    case AWAY__STATUS_WAITING:
    case AWAY__STATUS_WORKING:
    case AWAY__STATUS_SYNCING:
        return false;
    }

    if (node_id != away->expected_node_id && away->accept_counter--)
        return false;

    away->accept_counter = AWAY__ACCEPT_COUNTER;
    away__change_expected_id(node_id);
    return true;
}

_Bool ti_away_is_working(void)
{
    return away->status == AWAY__STATUS_WORKING;
}

int ti_away_syncer(ti_stream_t * stream, uint64_t first, uint64_t until)
{
    ti_syncer_t * syncer;
    ti_syncer_t ** empty_syncer = NULL;

    for (vec_each(away->syncers, ti_syncer_t, syncr))
    {
        if (syncr->stream == stream)
        {
            syncr->first = first;
            syncr->until = until;
            return 0;
        }
        if (!syncr->stream)
        {
            empty_syncer = v__;
        }
    }

    if (empty_syncer)
    {
        syncer = *empty_syncer;
        syncer->stream = stream;
        syncer->first = first;
        syncer->until = until;
        goto finish;
    }

    syncer = ti_syncer_create(stream, first, until);
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

static void away__destroy(uv_handle_t * handle)
{
    if (away)
    {
        free(away->work);
        free(away->repeat);
        /* only call free on timer if we have a handle since it otherwise will
         * be removed by ti_away_stop()
         */
        if (!handle)
            free(away->waiter);

        vec_destroy(away->syncers, (vec_destroy_cb) ti_watch_drop);
        free(away);
    }
    away = ti()->away = NULL;
}

static void away__req_away_id(void)
{
    vec_t * vec_nodes = ti()->nodes->vec;
    ti_quorum_t * quorum = NULL;
    ti_pkg_t * pkg, * dup;
    ti_node_t * this_node = ti()->node;
    quorum = ti_quorum_new((ti_quorum_cb) away__on_req_away_id, NULL);
    if (!quorum)
        goto failed;

    pkg = ti_pkg_new(0, TI_PROTO_NODE_REQ_AWAY, NULL, 0);
    if (!pkg)
        goto failed;

    /* this is used in case of an equal result and should be a value > 0 */
    ti_quorum_set_id(quorum, this_node->id + 1);

    for (vec_each(vec_nodes, ti_node_t, node))
    {
        if (node == ti()->node)
            continue;

        dup = NULL;
        if (node->status <= TI_NODE_STAT_CONNECTING ||
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
    log_critical(EX_ALLOC_S);
    away->status = AWAY__STATUS_IDLE;
}

static _Bool away__required(void)
{
    return ti()->archive->queue->n || ti_nodes_require_sync();
}

static void away__on_req_away_id(void * UNUSED(data), _Bool accepted)
{
    if (!accepted)
    {
        log_info(
                "node `%s` does not have the required quorum "
                "of at least %u connected nodes for going into away mode",
                ti_node_name(ti()->node),
                ti_nodes_quorum());
        goto fail0;;
    }

    away__change_expected_id(ti()->node->id);

    if (uv_timer_init(ti()->loop, away->waiter))
        goto fail1;

    if (uv_timer_start(
            away->waiter,
            away__waiter_pre_cb,
            AWAY__SOON_TIMER,   /* x seconds we keep in AWAY_SOON mode */
            1000                /* a little longer if events are still queued */
    ))
        goto fail2;

    ti_set_and_broadcast_node_status(TI_NODE_STAT_AWAY_SOON);
    away->status = AWAY__STATUS_WAITING;
    return;

fail2:
    uv_close((uv_handle_t *) away->waiter, NULL);
fail1:
    log_critical(EX_INTERNAL_S);
fail0:
    away->status = AWAY__STATUS_IDLE;
}

static void away__change_expected_id(uint8_t node_id)
{
    uint64_t new_timer;

    /* update expected node id */
    away->expected_node_id = (node_id + 1) % ti()->nodes->vec->n;

    /* calculate new timer */
    new_timer = away__calc_sleep();

    uv_timer_set_repeat(away->repeat, new_timer);
}

static void away__waiter_pre_cb(uv_timer_t * waiter)
{
    if (ti()->events->queue->n)
    {
        log_warning(
                "waiting for %zu %s to finish before going to away mode",
                ti()->events->queue->n,
                ti()->events->queue->n == 1 ? "event" : "events");
        ti_events_trigger_loop();
        return;
    }

    (void) uv_timer_stop(waiter);
    uv_close((uv_handle_t *) waiter, NULL);

    if (uv_queue_work(
            ti()->loop,
            away->work,
            away__work,
            away__work_finish))
    {
        away->status = AWAY__STATUS_IDLE;
        ti_set_and_broadcast_node_status(TI_NODE_STAT_READY);
        return;
    }

    ti_set_and_broadcast_node_status(TI_NODE_STAT_AWAY);
}

static size_t away__syncers(void)
{
    size_t count = 0;
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

            if (syncer->first < ti()->archive->first_event_id)
            {
                log_info("full database sync is required for `%s`",
                        ti_stream_name(syncer->stream));
                if (ti_syncfull_start(syncer->stream))
                    log_critical(EX_ALLOC_S);
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
                log_critical(EX_ALLOC_S);
            }
        }
    }
    return count;
}

static void away__waiter_after_cb(uv_timer_t * waiter)
{
    assert (away->status == AWAY__STATUS_SYNCING);

    size_t nsyncers, queue_size = ti()->events->queue->n;

    /*
     * First check and process events before start with synchronizing
     * optional nodes. This order is required so nodes will receive events
     * from the archive queue which they might require.
     */
    if (queue_size)
    {
        log_warning(
                "stay in away mode since the queue contains %zu %s",
                queue_size,
                queue_size == 1 ? "event" : "events");
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
    uv_close((uv_handle_t *) waiter, NULL);

    away->status = AWAY__STATUS_IDLE;
    ti_set_and_broadcast_node_status(TI_NODE_STAT_READY);
}

static void away__work(uv_work_t * UNUSED(work))
{
    uv_mutex_lock(ti()->events->lock);

    away->status = AWAY__STATUS_WORKING;

    /* garbage collect dropped collections */
    (void) ti_collections_gc_collect_dropped();

    for (vec_each(ti()->collections->vec, ti_collection_t, collection))
    {
        uv_mutex_lock(collection->lock);

        if (ti_things_gc(collection->things, collection->root))
        {
            log_error("garbage collection for collection `%.*s` has failed",
                    (int) collection->name->n,
                    (char *) collection->name->data);
        }

        uv_mutex_unlock(collection->lock);
    }

    if (ti_archive_to_disk())
        log_critical("failed writing archived events to disk");

    if (ti_archive_write_nodes_scevid())
        log_warning(
                "failed writing last nodes committed to disk: "TI_EVENT_ID,
                ti()->events->cevid);

    ti_archive_cleanup();

    uv_mutex_unlock(ti()->events->lock);
}

static void away__work_finish(uv_work_t * UNUSED(work), int status)
{
    away->status = AWAY__STATUS_SYNCING;

    int rc;
    if (status)
        log_error(uv_strerror(status));

    rc = uv_timer_init(ti()->loop, away->waiter);
    if (rc)
        goto fail1;

    rc = uv_timer_start(
            away->waiter,
            away__waiter_after_cb,
            2000,       /* give the system a few seconds to process events */
            5000        /* check on repeat if finished */
    );

    if (rc)
        goto fail2;

    return;

fail2:
    uv_close((uv_handle_t *) away->waiter, NULL);

fail1:
    log_error("cannot start `away` waiter: `%s`", uv_strerror(rc));
    away->status = AWAY__STATUS_IDLE;
    ti_set_and_broadcast_node_status(TI_NODE_STAT_READY);
}

static inline void away__repeat_cb(uv_timer_t * UNUSED(repeat))
{
    ti_events_trigger_loop();
    ti_away_trigger();
}

static inline uint64_t away__calc_sleep(void)
{
    return 2500 + ((away->expected_node_id < ti()->node->id
            ? away->expected_node_id + ti()->nodes->vec->n
            : away->expected_node_id) - ti()->node->id) * 11000;
}
