/*
 * ti/events.c
 */
#include <assert.h>
#include <qpack.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/event.h>
#include <ti/events.h>
#include <ti/quorum.h>
#include <ti/proto.h>
#include <util/fx.h>
#include <util/logger.h>
#include <util/qpx.h>
#include <util/vec.h>
#include <util/util.h>

/*
 * If an event is in the queue for this time, continue regardless of the event
 * status.
 */
#define EVENTS__TIMEOUT 42.0f

/*
 * Avoid extreme gaps between event id's
 */
#define EVENTS__MAX_ID_GAP  10000

static ti_events_t * events;

static void events__destroy(uv_handle_t * UNUSED(handle));
static void events__new_id(ti_event_t * ev);
static int events__req_event_id(ti_event_t * ev, ex_t * e);
static void events__on_req_event_id(ti_event_t * ev, _Bool accepted);
static int events__push(ti_event_t * ev);
static void events__loop(uv_async_t * handle);
static inline int events__trigger(void);
static inline _Bool events__max_id_gap(uint64_t event_id);

/*
 * Returns 0 on success
 * - creates singleton `events`
 */
int ti_events_create(void)
{
    events = ti()->events = malloc(sizeof(ti_events_t));
    if (!events)
        goto failed;

    events->is_started = false;
    events->queue = queue_new(4);
    events->evloop = malloc(sizeof(uv_async_t));
    events->lock = malloc(sizeof(uv_mutex_t));
    events->next_event_id = 0;

    if (!events->lock || uv_mutex_init(events->lock))
    {
        log_critical("failed to initiate uv_mutex lock");
        goto failed;
    }

    if (!events->queue || !events->evloop)
        goto failed;

    return 0;

failed:
    ti_events_stop();
    return -1;
}

/*
 * Returns 0 on success
 * - initialize and start `events`
 */
int ti_events_start(void)
{
    if (uv_async_init(ti()->loop, events->evloop, events__loop))
        return -1;
    events->is_started = true;
    return 0;
}

/*
 * Stop and destroy `events`
 */
void ti_events_stop(void)
{
    if (!events)
        return;

    if (events->is_started)
        uv_close((uv_handle_t *) events->evloop, events__destroy);
    else
        events__destroy(NULL);
}

int ti_events_trigger_loop(void)
{
    return events__trigger();
}

int ti_events_on_event(ti_node_t * from_node, ti_pkg_t * pkg)
{
    int rc;
    ti_epkg_t * epkg = ti_epkg_from_pkg(pkg);
    if (!epkg)
        return -1;

    rc = ti_events_add_event(from_node, epkg);
    ti_epkg_drop(epkg);

    return rc;
}

int ti_events_create_new_event(ti_query_t * query, ex_t * e)
{
    ti_event_t * ev;

    if (!ti_nodes_has_quorum())
    {
        uint8_t quorum = ti_nodes_quorum();
        ex_set(e, EX_NODE_ERROR,
                "node `%s` does not have the required quorum "
                "of at least %u connected %s",
                ti()->hostname,
                quorum, quorum == 1 ? "node" : "nodes");
        return e->nr;
    }

    if (queue_reserve(&events->queue, 1))
    {
        ex_set_alloc(e);
        return e->nr;
    }

    ev = ti_event_create(TI_EVENT_TP_MASTER);
    if (!ev)
    {
        ex_set_alloc(e);
        return e->nr;
    }

    ev->via.query = query;
    query->ev = ti_grab(ev);
    ev->target = ti_grab(query->target);

    return events__req_event_id(ev, e);
}

/*
 * Returns 0 on success, < 0 when critical, > 0 if not added for a reason
 * - function performs logging on failure
 * - `node` is required but only used for logging
 * - increments a reference for `epkg` on success
 */
int ti_events_add_event(ti_node_t * node, ti_epkg_t * epkg)
{
    ti_event_t * ev = NULL;

    if (events__max_id_gap(epkg->event_id))
    {
        log_error(
                TI_EVENT_ID" is too high compared to "
                "the next expected "TI_EVENT_ID,
                epkg->event_id,
                events->next_event_id);
        return 1;
    }

    /*
     * Update the event id in an early stage, this should be done even if
     * something else fails
     */
    if (epkg->event_id >= events->next_event_id)
        events->next_event_id = epkg->event_id + 1;

    for (queue_each(events->queue, ti_event_t, event))
        if (event->id == epkg->event_id)
            ev = event;

    if (!ev)
    {
        if (queue_reserve(&events->queue, 1))
            return -1;

        ev = ti_event_create(TI_EVENT_TP_EPKG);
        if (!ev)
            return -1;

        ev->via.epkg = ti_grab(epkg);
        ev->id = epkg->event_id;

        /* we have space so this function always succeeds */
        (void) events__push(ev);

        goto done;
    }

    if (ev->status == TI_EVENT_STAT_READY)
    {
        assert (ev->tp != TI_EVENT_TP_SLAVE);

        log_warning(
            TI_EVENT_ID" is being processed and "
            "can not be reused for node `%s`",
            ev->id,
            ti_node_name(node)
        );

        return 1;
    }

    assert (ev->tp != TI_EVENT_TP_EPKG);

    if (ev->tp == TI_EVENT_TP_SLAVE)
    {
        if (ev->via.node != node)
        {
            log_info(
                TI_EVENT_ID" was create for node `%s` but is now "
                "reused by an event from node `%s`",
                ev->id,
                ti_node_name(ev->via.node),
                ti_node_name(node)
            );
        }
    }
    ti_node_drop(ev->via.node);

    ev->tp = TI_EVENT_TP_EPKG;
    ev->status = TI_EVENT_STAT_READY;
    ev->via.epkg = ti_grab(epkg);

done:
    ev->status = TI_EVENT_STAT_READY;

    if (events__trigger())
        log_error("cannot trigger the events loop");

    return 0;
}

static ti_event_t * events__slave_new(ti_node_t * node, uint64_t event_id)
{
    assert (event_id >= events->next_event_id);
    ti_event_t * ev;

    if (queue_reserve(&events->queue, 1))
    {
        log_critical(EX_ALLOC_S);
        return NULL;
    }

    ev = ti_event_create(TI_EVENT_TP_SLAVE);
    if (!ev)
    {
        log_critical(EX_ALLOC_S);
        return NULL;
    }

    ev->id = event_id;
    ev->via.node = ti_grab(node);
    events->next_event_id = event_id + 1;

    (void) events__push(ev);

    return ev;
}

/* Returns true if the event is accepted, false if not. In case the event
 * is not accepted due to an error, logging is done.
 */
_Bool ti_events_slave_req(ti_node_t * node, uint64_t event_id)
{
    if (event_id == events->next_event_id)
    {
        return events__slave_new(node, event_id) != NULL;
    }

    if (event_id > events->next_event_id)
    {
        log_debug(
            "next expected event is "TI_EVENT_ID" but received "TI_EVENT_ID,
            events->next_event_id, event_id);
        return events__slave_new(node, event_id) != NULL;
    }

    return false;
}

static void events__destroy(uv_handle_t * UNUSED(handle))
{
    if (!events)
        return;
    queue_destroy(events->queue, (queue_destroy_cb) ti_event_drop);
    uv_mutex_destroy(events->lock);
    free(events->lock);
    free(events->evloop);
    free(events);
    events = ti()->events = NULL;
}

static void events__new_id(ti_event_t * ev)
{
    ex_t * e = ex_use();

    /* remove the event from the queue */
    (void) queue_rmval(events->queue, ev);

    if (events__req_event_id(ev, e))
    {
        ti_query_send(ev->via.query, e);
        ev->status = TI_EVENT_STAT_CACNCEL;
    }
}

static int events__req_event_id(ti_event_t * ev, ex_t * e)
{
    assert (queue_space(events->queue) > 0);

    vec_t * vec_nodes = ti()->nodes->vec;
    ti_quorum_t * quorum;
    qpx_packer_t * packer;
    ti_pkg_t * pkg, * dup;

    quorum = ti_quorum_new((ti_quorum_cb) events__on_req_event_id, ev);
    if (!quorum)
    {
        ex_set_alloc(e);
        return e->nr;
    }

    packer = qpx_packer_create(9, 0);
    if (!packer)
    {
        ti_quorum_destroy(quorum);
        ex_set_alloc(e);
        return e->nr;
    }

    ti_incref(ev);
    ev->id = events->next_event_id;
    ++events->next_event_id;

    ti_quorum_set_id(quorum, ev->id);
    (void) qp_add_int(packer, ev->id);
    pkg = qpx_packer_pkg(packer, TI_PROTO_NODE_REQ_EVENT_ID);

    /* we have space so this function always succeeds */
    (void) events__push(ev);

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
                TI_PROTO_NODE_REQ_EVENT_ID_TIMEOUT,
                ti_quorum_req_cb,
                quorum))
        {
            free(dup);
            if (ti_quorum_shrink_one(quorum))
                log_error("failed to reach quorum while the previous check"
                        "was successful");
        }
    }

    free(pkg);

    ti_quorum_go(quorum);

    return 0;
}

static void events__on_req_event_id(ti_event_t * ev, _Bool accepted)
{
    if (!accepted)
    {
        ++ti()->counters->events_quorum_lost;

        if (!ti_nodes_has_quorum())
        {
            ex_t * e = ex_use();
            ev->status = TI_EVENT_STAT_CACNCEL;

            ex_set(e, EX_NODE_ERROR,
                    "node `%s` does not have the required quorum "
                    "of at least %u connected nodes",
                    ti_node_name(ti()->node),
                    ti_nodes_quorum());
            ti_query_send(ev->via.query, e);
            goto done;
        }

        events__new_id(ev);
        goto done;
    }

    ev->status = TI_EVENT_STAT_READY;
    if (events__trigger())
        log_error("cannot trigger the events loop");

done:
    ti_event_drop(ev);
}

static int events__push(ti_event_t * ev)
{
    size_t idx = 0;
    ti_event_t * last_ev = queue_last(events->queue);

    if (!last_ev || ev->id > last_ev->id)
        return queue_push(&events->queue, ev);

    ++ti()->counters->events_unaligned;
    for (queue_each(events->queue, ti_event_t, event), ++idx)
        if (event->id > ev->id)
            break;

    return queue_insert(&events->queue, idx, ev);
}

static void events__loop(uv_async_t * UNUSED(handle))
{
    ti_event_t * ev;
    util_time_t timing;
    uint64_t * cevid_p = &ti()->node->cevid;

    if (uv_mutex_trylock(events->lock))
        return;

    if (clock_gettime(TI_CLOCK_MONOTONIC, &timing))
        goto stop;

    while ((ev = queue_first(events->queue)))
    {
        if (ev->id <= *cevid_p)
        {
            /* cancelled events which are `skipped` can be removed */
            if (ev->status != TI_EVENT_STAT_CACNCEL)
            {
                log_error(
                        TI_EVENT_ID" will be skipped because "TI_EVENT_ID
                        " is already committed",
                        ev->id, *cevid_p);
                ti_event_log("skipped", ev);

                ++ti()->counters->events_skipped;
            }

            goto shift_drop_loop;
        }
        else if (ev->id > (ti()->node->cevid) + 1)
        {
            /* We expect at least one event before this one */
            if (ti()->node->status == TI_NODE_STAT_SYNCHRONIZING ||
                util_time_diff(&ev->time, &timing) < EVENTS__TIMEOUT)
                break;

            ++ti()->counters->events_with_gap;
            /* Reached time-out, continue */
        }

        if (    ev->status == TI_EVENT_STAT_CACNCEL ||
                ev->status == TI_EVENT_STAT_NEW)
        {
            /* An event must have status READY before we can continue,
             * cancelled events can be replaced with valid events so wait for
             * those too
             */
            if (util_time_diff(&ev->time, &timing) < EVENTS__TIMEOUT)
                break;

            if (ev->status == TI_EVENT_STAT_NEW)
            {
                log_error(
                        "kill "TI_EVENT_ID" on node `%s` "
                        "in approximately %f seconds",
                        ev->id,
                        ti_name(),
                        util_time_diff(&ev->time, &timing));
            }

            /* Reached time-out, kill the event */
            ++ti()->counters->events_killed;

            goto shift_drop_loop;
        }

        assert (ev->status == TI_EVENT_STAT_READY);

        switch ((ti_event_tp_enum) ev->tp)
        {
        case TI_EVENT_TP_MASTER:
            assert (ev->status == TI_EVENT_STAT_READY);
            ti_query_run(ev->via.query);
            break;
        case TI_EVENT_TP_EPKG:
            assert (ev->tp == TI_EVENT_TP_EPKG);
            if (ti_archive_push(ev->via.epkg) || ti_event_run(ev))
            {
                /* logging is done, but we increment the failed counter and
                 * log the full event */
                ++ti()->counters->events_failed;
                ti_event_log("failed", ev);
            }
            break;
        case TI_EVENT_TP_SLAVE:
            assert (0);
        }

        if (Logger.level == LOGGER_DEBUG)
            ti_event_log("processed", ev);

        /* update counters */
        ti_counters_upd_commit_event(&ev->time);

        /* update committed event id */
        *cevid_p = ev->id;

shift_drop_loop:
        (void) queue_shift(events->queue);
        ti_event_drop(ev);
    }

stop:
    uv_mutex_unlock(events->lock);
}

static inline int events__trigger(void)
{
    return uv_async_send(events->evloop);
}

static inline _Bool events__max_id_gap(uint64_t event_id)
{
    return (
        event_id > events->next_event_id &&
        event_id - events->next_event_id > EVENTS__MAX_ID_GAP
    );
}
