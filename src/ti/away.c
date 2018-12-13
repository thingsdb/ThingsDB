/*
 * ti/away.c
 */
#include <assert.h>
#include <ti/away.h>
#include <ti/proto.h>
#include <ti/quorum.h>
#include <ti/things.h>
#include <ti.h>
#include <util/logger.h>
#include <util/qpx.h>
#include <util/util.h>
#include <uv.h>

static ti_away_t * away;

enum
{
    AWAY__FLAG_IS_STARTED       =1<<0,
    AWAY__FLAG_IS_RUNNING       =1<<1,
    AWAY__FLAG_IS_WAITING       =1<<2,
    AWAY__FLAG_IS_WORKING       =1<<3,
};

static void away__destroy(uv_handle_t * handle);
static void away__req_away_id(void);
static void away__on_req_away_id(void * UNUSED(data), _Bool accepted);
static void away__waiter_cb(uv_timer_t * timer);
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
    away->flags = 0;
    away->id = 0;

    if (!away->work || !away->repeat || !away->waiter)
    {
        away__destroy(NULL);
        return -1;
    }

    ti()->away = away;
    return 0;
}

int ti_away_start(void)
{
    assert (away->flags == 0);

    if (uv_timer_init(ti()->loop, away->repeat))
        goto fail0;

    if (uv_timer_start(away->repeat,
            away__repeat_cb,
            away__calc_sleep(),
            away__calc_sleep()))
        goto fail1;

    away->flags |= AWAY__FLAG_IS_STARTED;
    return 0;

fail1:
    uv_close((uv_handle_t *) away->repeat, away__destroy);
fail0:
    return -1;
}

void ti_away_trigger(void)
{
    if ((away->flags & AWAY__FLAG_IS_RUNNING) ||
        (~ti()->node->flags & TI_NODE_STAT_READY) ||
        ti_nodes_get_away_or_soon())
        return;

    away->flags |= AWAY__FLAG_IS_RUNNING;
    away__req_away_id();
}

void ti_away_stop(void)
{
    if (!away)
        return;

    if (~away->flags & AWAY__FLAG_IS_STARTED)
        away__destroy(NULL);
    else
    {
        if (~away->flags & AWAY__FLAG_IS_WAITING)
            free(away->waiter);
        else
        {
            uv_timer_stop(away->waiter);
            uv_close((uv_handle_t *) away->waiter, (uv_close_cb) free);
        }

        uv_timer_stop(away->repeat);
        uv_close((uv_handle_t *) away->repeat, away__destroy);
    }
}

_Bool ti_away_is_working(void)
{
    return away->flags & AWAY__FLAG_IS_WORKING;
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
    ti_quorum_set_id(quorum, 1);
    (void) qp_add_int64(packer, away->id);
    pkg = qpx_packer_pkg(packer, TI_PROTO_NODE_REQ_EVENT_ID);

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
            if (ti_quorum_shrink_one(quorum))
                log_error(
                        "failed to reach quorum while the previous check "
                        "was successful");
        }
    }

    if (!quorum->sz)
        free(pkg);

    ti_quorum_go(quorum);

    return;

failed:
    ti_quorum_destroy(quorum);
    log_critical(EX_ALLOC_S);
    away->flags &= ~AWAY__FLAG_IS_RUNNING;
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

    assert (away->flags & AWAY__FLAG_IS_STARTED);
    assert (away->flags & AWAY__FLAG_IS_RUNNING);
    assert (~away->flags & AWAY__FLAG_IS_WAITING);

    uv_timer_set_repeat(away->repeat, away__calc_sleep());

    if (uv_timer_init(ti()->loop, away->waiter))
        goto fail1;

    if (uv_timer_start(
            away->waiter,
            away__waiter_cb,
            10000,      /* for 10 seconds we keep in AWAY_SOON mode */
            1000        /* a little longer if events are still queued */
    ))
        goto fail2;

    ti_change_and_broadcast_node_status(TI_NODE_STAT_AWAY_SOON);
    away->flags |= AWAY__FLAG_IS_WAITING;
    return;

fail2:
    uv_close((uv_handle_t *) away->waiter, NULL);
fail1:
    log_critical(EX_INTERNAL_S);
fail0:
    away->flags &= ~AWAY__FLAG_IS_RUNNING;
}

static void away__waiter_cb(uv_timer_t * waiter)
{
    assert (ti()->node->status == TI_NODE_STAT_AWAY_SOON);
    assert (away->flags & AWAY__FLAG_IS_STARTED);
    assert (away->flags & AWAY__FLAG_IS_RUNNING);
    assert (away->flags & AWAY__FLAG_IS_WAITING);

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

    away->flags &= ~AWAY__FLAG_IS_WAITING;

    if (uv_queue_work(
            ti()->loop,
            away->work,
            away__work,
            away__work_finish))
    {
        away->flags &= ~AWAY__FLAG_IS_RUNNING;
        ti_change_and_broadcast_node_status(TI_NODE_STAT_READY);
        uv_mutex_unlock(ti()->events->lock);
        return;
    }

    ti_change_and_broadcast_node_status(TI_NODE_STAT_AWAY);
}

static void away__work(uv_work_t * UNUSED(work))
{
    away->flags |= AWAY__FLAG_IS_WORKING;

    if (ti_sleep(250) == 0)
    {
        /* this small sleep gives the system time to set away mode */
        assert (ti()->node->status == TI_NODE_STAT_AWAY);
    }
    assert (away->flags & AWAY__FLAG_IS_STARTED);
    assert (away->flags & AWAY__FLAG_IS_RUNNING);
    assert (~away->flags & AWAY__FLAG_IS_WAITING);

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
    assert (away->flags & AWAY__FLAG_IS_STARTED);
    assert (away->flags & AWAY__FLAG_IS_RUNNING);
    assert (~away->flags & AWAY__FLAG_IS_WAITING);

    away->flags &= ~AWAY__FLAG_IS_WORKING;
    away->flags &= ~AWAY__FLAG_IS_RUNNING;

    uv_mutex_unlock(ti()->events->lock);

    if (status)
        log_error(uv_strerror(status));

    if (ti()->flags & TI_FLAG_SIGNAL)
    {
        if (ti()->flags & TI_FLAG_STOP)
            ti_stop();
        else
            ti_stop_slow();
        return;
    }

    ti_change_and_broadcast_node_status(TI_NODE_STAT_READY);
}

static inline void away__repeat_cb(uv_timer_t * UNUSED(repeat))
{
    ti_away_trigger();
}

static inline uint64_t away__calc_sleep(void)
{
    return ti()->nodes->vec->n == 1 ?
            3600000L :   /* once an hour: 3600000L */
            ((away->id + ti()->node->id) % ti()->nodes->vec->n) * 5000 + 1000;
}
