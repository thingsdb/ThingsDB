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
    AWAY__FLAG_CAN_CANCELLED    =1<<3,
};

static void away__destroy(uv_handle_t * handle);
static void away__repeat_cb(uv_timer_t * repeat);
static void away__req_away_id(void);
static void away__on_req_away_id(void * UNUSED(data), _Bool accepted);
static void away__waiter_cb(uv_timer_t * timer);
static void away__work(uv_work_t * UNUSED(work));
static void away__work_finish(uv_work_t * UNUSED(work), int status);
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

/* Returns true if away mode is cancelled, false if thread was not running */
_Bool ti_away_cancel(void)
{
    return away->flags & AWAY__FLAG_CAN_CANCELLED
            ? uv_cancel((uv_req_t *) away->work)
            : 0;
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

static void away__repeat_cb(uv_timer_t * UNUSED(repeat))
{
    if ((away->flags & AWAY__FLAG_IS_RUNNING) || ti_nodes_get_away_or_soon())
        return;

    away->flags |= AWAY__FLAG_IS_RUNNING;
    away__req_away_id();
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
                log_error("failed to reach quorum while the previous check"
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
        log_error("node `%s` does not have the required quorum "
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

    ti_set_and_send_node_status(TI_NODE_STAT_AWAY_SOON);
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
                "waiting for %zu events to finish before going to away mode",
                ti()->events->queue->n);
        return;
    }

    (void) uv_timer_stop(waiter);
    uv_close((uv_handle_t *) waiter, NULL);

    away->flags &= ~AWAY__FLAG_IS_WAITING;

    if (uv_queue_work(
            ti()->loop,
            away->work,
            away__work,
            away__work_finish))
    {
        away->flags &= ~AWAY__FLAG_IS_RUNNING;
        ti_set_and_send_node_status(TI_NODE_STAT_READY);
        return;
    }
    away->flags |= AWAY__FLAG_CAN_CANCELLED;
    ti_set_and_send_node_status(TI_NODE_STAT_AWAY);
}

static void away__work(uv_work_t * UNUSED(work))
{
    (void) util_sleep(500);

    assert (ti()->node->status == TI_NODE_STAT_AWAY);
    assert (away->flags & AWAY__FLAG_IS_STARTED);
    assert (away->flags & AWAY__FLAG_IS_RUNNING);
    assert (~away->flags & AWAY__FLAG_IS_WAITING);

    for (vec_each(ti()->dbs, ti_db_t, db))
    {
        uv_mutex_lock(db->lock);

        if (ti_things_gc(db->things, db->root, true))
        {
            log_error("garbage collection for database `%.*s` has failed",
                    (int) db->name->n, (char *) db->name->data);
        }

        uv_mutex_unlock(db->lock);
    }

    if (ti_archive_to_disk(true))
        log_critical("failed writing archived events to disk");

    ti_archive_cleanup(true);
}

static void away__work_finish(uv_work_t * UNUSED(work), int status)
{
    assert (away->flags & AWAY__FLAG_IS_STARTED);
    assert (away->flags & AWAY__FLAG_IS_RUNNING);
    assert (~away->flags & AWAY__FLAG_IS_WAITING);

    LOGC("Status: %d", status);

    if (status == UV_ECANCELED)
    {
        ti_stop();
        return;
    }
    else if (status)
    {
        log_error(uv_strerror(status));
    }
    ti_set_and_send_node_status(TI_NODE_STAT_READY);
    away->flags &= ~AWAY__FLAG_IS_RUNNING;
}

static inline uint64_t away__calc_sleep(void)
{
    return ti()->nodes->vec->n == 1 ?
            12000L :   /* 120000L */
            ((away->id + ti()->node->id) % ti()->nodes->vec->n) * 5000 + 1000;
}
