/*
 * ti/away.c
 */
#include <assert.h>
#include <ti/away.h>
#include <ti/proto.h>
#include <ti/quorum.h>
#include <ti/things.h>
#include <ti/fsync.h>
#include <ti/syncer.h>
#include <ti.h>
#include <util/logger.h>
#include <util/qpx.h>
#include <util/util.h>
#include <uv.h>

static ti_away_t * away = NULL;

/*
 * When `away` mode is finished we might have some events queued, after the
 * queue has less or equal to this threshold we can go out of away mode
 */
#define AWAY__THRESHOLD_EVENTS_QUEUE_SIZE 5
#define AWAY__ACCEPT_COUNTER 3
#define AWAY__SOON_TIMER 1000  /* TODO: 10 seconds might be a nice default */

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
static void away__on_req_away_id(void * UNUSED(data), _Bool accepted);
static void away__waiter_pre_cb(uv_timer_t * timer);
static void away__waiter_after_cb(uv_timer_t * waiter);
static void away__work(uv_work_t * UNUSED(work));
static void away__work_finish(uv_work_t * UNUSED(work), int status);
static inline void away__repeat_cb(uv_timer_t * repeat);
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
    away->id = 0;

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
    if (away->accept_counter)
        --away->accept_counter;

    if ((away->status != AWAY__STATUS_IDLE) ||
        ti()->node->status != TI_NODE_STAT_READY ||
        ti()->nodes->vec->n == 1 ||
        !ti_nodes_has_quorum() ||
        ti_nodes_get_away_or_soon())
    {
        log_debug("not going in away mode (%s)",
                away->status != AWAY__STATUS_IDLE
                ? "away status is not idle"
                : ti()->node->status != TI_NODE_STAT_READY
                ? "node status is not ready"
                : ti()->nodes->vec->n == 1
                ? "cluster has only one node"
                : !ti_nodes_has_quorum()
                ? "quorum not reached"
                : "other node is away or going away soon");
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
        away__destroy(NULL);
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

_Bool ti_away_accept(uint8_t node_id, uint8_t away_id)
{
    switch ((enum away__status) away->status)
    {
    case AWAY__STATUS_INIT:
    case AWAY__STATUS_IDLE:
        break;
    case AWAY__STATUS_REQ_AWAY:
        if (node_id < ti()->node->id)
            return false;
        break;
    case AWAY__STATUS_WAITING:
    case AWAY__STATUS_WORKING:
    case AWAY__STATUS_SYNCING:
        return false;
    }

    if (away->accept_counter)
        return false;

    away->id = away_id;
    away->accept_counter = AWAY__ACCEPT_COUNTER;
    return true;
}

_Bool ti_away_is_working(void)
{
    return away->status == AWAY__STATUS_WORKING;
}

int ti_away_syncer(ti_stream_t * stream, uint64_t start, uint64_t until)
{
    ti_syncer_t * syncer;
    ti_syncer_t ** empty_syncer = NULL;


    for (vec_each(away->syncers, ti_syncer_t, syncr))
    {
        if (syncr->stream == stream)
        {
            syncr->start = start;
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
        syncer->start = start;
        syncer->until = until;
        goto finish;
    }

    syncer = ti_syncer_create(stream, start, until);
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

    LOGC("GOT I: %zu", i);

    if (i < away->syncers->n)
    {
        LOGC("REMOVED...");
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
    qpx_packer_t * packer;
    ti_pkg_t * pkg;

    quorum = ti_quorum_new((ti_quorum_cb) away__on_req_away_id, NULL);
    if (!quorum)
        goto failed;

    packer = qpx_packer_create(2, 0);
    if (!packer)
        goto failed;

    away->id++;
    away->id %= vec_nodes->n;

    /* this is used in case of an equal result and should be a value > 0 */
    ti_quorum_set_id(quorum, away->id + 1);
    (void) qp_add_int(packer, away->id);
    pkg = qpx_packer_pkg(packer, TI_PROTO_NODE_REQ_AWAY_ID);

    for (vec_each(vec_nodes, ti_node_t, node))
    {
        if (node == ti()->node)
            continue;

        if (node->status <= TI_NODE_STAT_CONNECTING || ti_req_create(
                node->stream,
                pkg,
                TI_PROTO_NODE_REQ_AWAY_ID_TIMEOUT,
                ti_quorum_req_cb,
                quorum))
        {
            LOGC("Status: %d", node->status);

            if (ti_quorum_shrink_one(quorum))
                log_error(
                    "failed to reach quorum of %u nodes while the previous "
                    "check was successful", quorum->quorum);
        }
    }

    if (!quorum->sz)
        free(pkg);

    ti_quorum_go(quorum);

    return;

failed:
    ti_quorum_destroy(quorum);
    log_critical(EX_ALLOC_S);
    away->status = AWAY__STATUS_IDLE;
}

static void away__on_req_away_id(void * UNUSED(data), _Bool accepted)
{
    if (!accepted)
    {
        log_error(
                "node `%s` does not have the required quorum "
                "of at least %u connected nodes for going into away mode",
                ti_node_name(ti()->node),
                ti_nodes_quorum());
        goto fail0;;
    }


    assert (away->status == AWAY__STATUS_REQ_AWAY);

    uv_timer_set_repeat(away->repeat, away__calc_sleep());

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

static void away__waiter_pre_cb(uv_timer_t * waiter)
{
    if (ti()->events->queue->n)
    {
        log_warning(
                "waiting for %zu %s to finish before going to away mode",
                ti()->events->queue->n,
                ti()->events->queue->n == 1 ? "event" : "events");
        return;
    }

    (void) uv_timer_stop(waiter);
    uv_close((uv_handle_t *) waiter, NULL);

    uv_mutex_lock(ti()->events->lock);
    if (uv_queue_work(
            ti()->loop,
            away->work,
            away__work,
            away__work_finish))
    {
        away->status = AWAY__STATUS_IDLE;
        ti_set_and_broadcast_node_status(TI_NODE_STAT_READY);
        uv_mutex_unlock(ti()->events->lock);
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
            ++count;
            if (syncer->stream->flags & TI_STREAM_FLAG_SYNCHRONIZING)
                continue;

            syncer->stream->flags |= TI_STREAM_FLAG_SYNCHRONIZING;
            if (syncer->start < ti()->archive->start_event_id)
            {
                log_info("full database sync is required for `%s`",
                        ti_stream_name(syncer->stream));
                ti_fsync_start(syncer->stream);
            }
        }
    }
    return count;
}

static void away__waiter_after_cb(uv_timer_t * waiter)
{
    assert (away->status == AWAY__STATUS_SYNCING);

    size_t nsyncers = away__syncers();
    if (nsyncers)
    {
        log_warning(
                "stay in away mode since this node is synchronizing with "
                "%zu other %s",
                nsyncers,
                nsyncers ? "node" : "nodes");
        return;
    }

    if (ti()->events->queue->n > AWAY__THRESHOLD_EVENTS_QUEUE_SIZE)
    {
        log_warning(
                "stay in away mode since there are still %zu %s in the "
                "queue, a value of %u or lower is required",
                ti()->events->queue->n,
                ti()->events->queue->n == 1 ? "event" : "events",
                AWAY__THRESHOLD_EVENTS_QUEUE_SIZE);
        return;
    }

    (void) uv_timer_stop(waiter);
    uv_close((uv_handle_t *) waiter, NULL);

    away->status = AWAY__STATUS_IDLE;
    ti_set_and_broadcast_node_status(TI_NODE_STAT_READY);
}

static void away__work(uv_work_t * UNUSED(work))
{
    away->status = AWAY__STATUS_WORKING;

    if (ti_sleep(250) == 0)
    {
        /* this small sleep gives the system time to set away mode */
        assert (ti()->node->status == TI_NODE_STAT_AWAY);
    }

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
}

static void away__work_finish(uv_work_t * UNUSED(work), int status)
{
    away->status = AWAY__STATUS_SYNCING;

    int rc;
    if (status)
        log_error(uv_strerror(status));

    uv_mutex_unlock(ti()->events->lock);

    if (ti()->flags & TI_FLAG_SIGNAL)
    {
        away->status = AWAY__STATUS_IDLE;

        if (ti()->flags & TI_FLAG_STOP)
            ti_stop();
        else
            ti_stop_slow();
        return;
    }

    rc = uv_timer_init(ti()->loop, away->waiter);
    if (rc)
        goto fail1;

    rc = uv_timer_start(
            away->waiter,
            away__waiter_after_cb,
            2000,       /* give the system a few seconds to process events and
                           look for registered synchronizers */
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
    ti_away_trigger();
}

static inline uint64_t away__calc_sleep(void)
{
    return ((away->id + ti()->node->id) % ti()->nodes->vec->n) * 5000 + 1000;
}
