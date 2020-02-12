/*
 * ti/events.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/event.h>
#include <ti/events.h>
#include <ti/proto.h>
#include <ti/quorum.h>
#include <util/fx.h>
#include <util/logger.h>
#include <util/mpack.h>
#include <util/util.h>
#include <util/vec.h>

/*
 * When waiting for a missing event for this amount of time, continue and
 * accept that the missing event is not received;
 */
#define EVENTS__TIMEOUT 42.0

/*
 * When a new event is created and no quorum is reach in this amount of time,
 * kill the event and continue;
 */
#define EVENTS__NEW_TIMEOUT 21.0

/*
 * When waiting for a missing event for this amount of time, do an attempt to
 * request the missing event to a random online node;
 */
#define EVENTS__MISSING_TIMEOUT 6.0

/*
 * Avoid extreme gaps between event id's
 */
#define EVENTS__MAX_ID_GAP 1000

/*
 *
 */
#define EVENTS__INIT_DROPPED_SZ 128

ti_events_t events_;
static ti_events_t * events;

static void events__destroy(uv_handle_t * UNUSED(handle));
static void events__new_id(ti_event_t * ev);
static int events__req_event_id(ti_event_t * ev, ex_t * e);
static void events__on_req_event_id(ti_event_t * ev, _Bool accepted);
static int events__push(ti_event_t * ev);
static void events__loop(uv_async_t * handle);

static inline ssize_t events__trigger(void)
{
    return events->queue->n
            ? (
                    uv_async_send(events->evloop)
                        ? -1                          /* error */
                        : (ssize_t) events->queue->n  /* current size */
            )
            : 0;
}

static inline _Bool events__max_id_gap(uint64_t event_id)
{
    return (
        event_id > events->next_event_id &&
        event_id - events->next_event_id > EVENTS__MAX_ID_GAP
    );
}

/*
 * Returns 0 on success
 * - creates singleton `events`
 */
int ti_events_create(void)
{
    events = &events_;

    events->is_started = false;
    events->keep_dropped = false;
    events->queue = queue_new(4);
    events->evloop = malloc(sizeof(uv_async_t));
    events->lock = malloc(sizeof(uv_mutex_t));
    events->next_event_id = 0;
    events->dropped = vec_new(EVENTS__INIT_DROPPED_SZ);
    events->skipped_ids = olist_create();
    memset(&events->wait_gap_time, 0, sizeof(events->wait_gap_time));
    events->wait_cevid = 0;

    if (!events->skipped_ids ||
        !events->lock ||
        uv_mutex_init(events->lock))
    {
        log_critical("failed to initiate uv_mutex lock");
        goto failed;
    }

    if (!events->queue || !events->evloop)
        goto failed;

    ti()->events = events;
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

/* Returns a negative number in case of an error, or else the queue size at
 * the time the trigger is started.
 */
ssize_t ti_events_trigger_loop(void)
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
        ex_set_mem(e);
        return e->nr;
    }

    ev = ti_event_create(TI_EVENT_TP_MASTER);
    if (!ev)
    {
        ex_set_mem(e);
        return e->nr;
    }

    ev->via.query = query;
    query->ev = ti_grab(ev);
    ev->collection = ti_grab(query->collection);

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

    if (epkg->event_id <= ti()->node->cevid)
    {
        log_warning(TI_EVENT_ID" is already committed", epkg->event_id);
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

    if (ev)
    {
        if (ev->status == TI_EVENT_STAT_READY)
        {
            switch ((ti_event_tp_enum) ev->tp)
            {
            case TI_EVENT_TP_MASTER:
                log_warning(
                     TI_EVENT_ID" is being processed and "
                     "can not be reused for node `%s`",
                     ev->id,
                     ti_node_name(node)
                 );
                 break;
            case TI_EVENT_TP_EPKG:
                ev->via.epkg->flags |= epkg->flags;
                break;
            }
            return 1;
        }

        assert (ev->tp != TI_EVENT_TP_EPKG);

        /* event is owned by MASTER and needs to stay with MASTER */
        ev = queue_rmval(events->queue, ev);
        if (ev)
            ti_event_drop(ev);

        /* bubble down and create a new event */
    }

    if (queue_reserve(&events->queue, 1))
        return -1;

    ev = ti_event_create(TI_EVENT_TP_EPKG);
    if (!ev)
        return -1;

    ev->via.epkg = ti_grab(epkg);
    ev->id = epkg->event_id;

    /* we have space so this function always succeeds */
    (void) events__push(ev);

    ev->status = TI_EVENT_STAT_READY;

    if (events__trigger() < 0)
        log_error("cannot trigger the events loop");

    return 0;
}

/* Returns true if the event is accepted, false if not. In case the event
 * is not accepted due to an error, logging is done.
 */
ti_proto_enum_t ti_events_accept_id(uint64_t event_id, uint8_t * n)
{
    uint64_t * cevid_p;
    olist_iter_t iter;

    if (event_id == events->next_event_id)
    {
        ++events->next_event_id;
        return TI_PROTO_NODE_RES_ACCEPT;
    }

    if (event_id > events->next_event_id)
    {
        log_info("skipped %u event id's while accepting "TI_EVENT_ID,
                event_id - events->next_event_id,
                event_id);
        do
        {
            if (olist_set(events->skipped_ids, events->next_event_id))
                log_error(EX_MEMORY_S);

        } while (++events->next_event_id < event_id);

        ++events->next_event_id;
        return TI_PROTO_NODE_RES_ACCEPT;
    }

    for (queue_each(events->queue, ti_event_t, ev))
    {
        if (ev->id > event_id)
            break;

        if (ev->id == event_id)
            return ev->tp == TI_EVENT_TP_MASTER && ((*n = ev->requests) || 1)
                    ? TI_PROTO_NODE_ERR_COLLISION
                    : TI_PROTO_NODE_ERR_REJECT;
    }

    cevid_p = &ti()->node->cevid;
    iter = olist_iter(events->skipped_ids);

    for (size_t n = events->skipped_ids->n; n--;)
    {
        uint64_t id = olist_iter_id(iter);
        iter = iter->next_;

        if (id <= *cevid_p)
        {
            olist_rm(events->skipped_ids, id);
            continue;
        }

        if (id == event_id)
        {
            olist_rm(events->skipped_ids, id);
            return TI_PROTO_NODE_RES_ACCEPT;
        }

        if (id > event_id)
            break;
    }

    return TI_PROTO_NODE_ERR_REJECT;
}

/* Sets the next missing event id, at least higher than the given event_id.
 *
 */
void ti_events_set_next_missing_id(uint64_t * event_id)
{
    uint64_t cevid = ti()->node->cevid;

    if (cevid > *event_id)
        *event_id = cevid;

    for (queue_each(events->queue, ti_event_t, ev))
    {
        if (ev->id <= *event_id)
            continue;

        if (ev->id == ++(*event_id))
            continue;

        return;
    }

    ++(*event_id);
}

void ti_events_free_dropped(void)
{
    ti_thing_t * thing;

    events->keep_dropped = false;

    while ((thing = vec_pop(events->dropped)))
        if (!thing->ref)
            ti_thing_destroy(thing);

    assert (events->dropped->n == 0);
}

/* Only call while not is use, so `n` must be zero */
int ti_events_resize_dropped(void)
{
    assert (events->dropped->n == 0);
    return events->dropped->sz == EVENTS__INIT_DROPPED_SZ
            ? 0
            : vec_resize(&events->dropped, EVENTS__INIT_DROPPED_SZ);
}

vec_t * ti_events_pkgs_from_queue(ti_thing_t * thing)
{
    uint64_t next_event = ti()->node->cevid;
    vec_t * pkgs = NULL;

    for (queue_each(events->queue, ti_event_t, ev))
    {
        if (ev->id != ++next_event)
            break;

        if (ev->flags & TI_EVENT_FLAG_WATCHED)
            (void) ti_event_append_pkgs(ev, thing, &pkgs);
    }
    return pkgs;
}

static void events__destroy(uv_handle_t * UNUSED(handle))
{
    if (!events)
        return;
    queue_destroy(events->queue, (queue_destroy_cb) ti_event_drop);
    uv_mutex_destroy(events->lock);
    free(events->lock);
    free(events->evloop);
    vec_destroy(events->dropped, (vec_destroy_cb) ti_thing_destroy);
    olist_destroy(events->skipped_ids);
    events = ti()->events = NULL;
}

static void events__new_id(ti_event_t * ev)
{
    ex_t e = {0};

    /* remove the event from the queue (if still there) and reserve one
     * space if nothing is removed */
    if (!queue_rmval(events->queue, ev))
    {
        ti_incref(ev);
        if (queue_reserve(&events->queue, 1))
        {
            ex_set_mem(&e);
            goto fail;
        }
    }

    if (!ti_nodes_has_quorum())
    {
        ex_set(&e, EX_NODE_ERROR,
                "node `%s` does not have the required quorum "
                "of at least %u connected nodes",
                ti_node_name(ti()->node),
                ti_nodes_quorum());
        goto fail;
    }

    if (events__req_event_id(ev, &e) == 0)
        return;

    /* in case of an error, `ev->id` is not changed */
fail:
    ti_event_drop(ev);  /* reference for the queue */
    (void) ti_query_send_response(ev->via.query, &e);
    ev->status = TI_EVENT_STAT_CACNCEL;
}

static int events__req_event_id(ti_event_t * ev, ex_t * e)
{
    assert (queue_space(events->queue) > 0);

    msgpack_packer pk;
    msgpack_sbuffer buffer;
    vec_t * nodes_vec = imap_vec(ti()->nodes->imap);
    ti_quorum_t * quorum;
    ti_pkg_t * pkg, * dup;

    quorum = ti_quorum_new((ti_quorum_cb) events__on_req_event_id, ev);
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

    ti_incref(ev);
    ev->id = events->next_event_id;
    ++events->next_event_id;

    msgpack_pack_uint64(&pk, ev->id);

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_NODE_REQ_EVENT_ID, buffer.size);

    /* we have space so this function always succeeds */
    (void) events__push(ev);

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
                TI_PROTO_NODE_REQ_EVENT_ID_TIMEOUT,
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

    ev->requests = quorum->requests;

    free(pkg);

    ti_quorum_go(quorum);

    return 0;
}

static void events__on_req_event_id(ti_event_t * ev, _Bool accepted)
{
    if (!accepted)
    {
        ++ti()->counters->events_quorum_lost;

        log_debug(TI_EVENT_ID" quorum lost :-(", ev->id);

        events__new_id(ev);
        goto done;
    }

    log_debug(TI_EVENT_ID" quorum win :-)", ev->id);

    ev->status = TI_EVENT_STAT_READY;
    if (events__trigger() < 0)
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

static void events__watch_queue(void)
{
    uint64_t next_event = ti()->node->cevid;

    for (queue_each(events->queue, ti_event_t, ev))
    {
        if (ev->id != ++next_event)
            return;

        if (ti_event_require_watch_upd(ev))
            (void) ti_event_watch(ev);
    }
}

static void events__loop(uv_async_t * UNUSED(handle))
{
    ti_event_t * ev;
    util_time_t timing;
    uint64_t * cevid_p = &ti()->node->cevid;
    int process_events = 5;

    if (uv_mutex_trylock(events->lock))
    {
        events__watch_queue();
        return;
    }

    if (clock_gettime(TI_CLOCK_MONOTONIC, &timing))
        goto stop;

    while (process_events-- && (ev = queue_first(events->queue)))
    {
        /* Cancelled event should be removed from the queue */
        assert (ev->status != TI_EVENT_STAT_CACNCEL);

        if (ev->id <= *cevid_p)
        {
            log_warning(
                TI_EVENT_ID" will be skipped because "TI_EVENT_ID
                " is already committed",
                ev->id, *cevid_p);

            ++ti()->counters->events_skipped;

            goto shift_drop_loop;
        }
        else if (ev->id > (*cevid_p) + 1)
        {
            double diff;

            /* continue if the event is allowed a gap */
            if (ev->tp == TI_EVENT_TP_EPKG &&
                (ev->via.epkg->flags & TI_EPKG_FLAG_ALLOW_GAP))
                goto process;

            /* wait if the node is synchronizing */
            if (ti()->node->status == TI_NODE_STAT_SYNCHRONIZING)
                break;

            if (events->wait_cevid != *cevid_p)
            {
                events->wait_cevid = *cevid_p;
                clock_gettime(TI_CLOCK_MONOTONIC, &events->wait_gap_time);
                break;
            }

            diff = util_time_diff(&events->wait_gap_time, &timing);

            if (diff > EVENTS__MISSING_TIMEOUT)
                ti_event_missing_event((*cevid_p) + 1);

            if (diff < EVENTS__TIMEOUT)
                break;

            ++(*cevid_p);
            ++ti()->counters->events_with_gap;

            log_warning(
                    "committed "TI_EVENT_ID" since the event is not received "
                    "within approximately %f seconds", *cevid_p, diff);

            if (ev->id > (*cevid_p) + 1)
                break;
        }

        if (ev->status == TI_EVENT_STAT_NEW)
        {
            double diff = util_time_diff(&ev->time, &timing);
            /*
             * An event must have status READY before we can continue;
             */
            if (diff < EVENTS__NEW_TIMEOUT)
                break;

            log_error(
                    "killed "TI_EVENT_ID" on node `%s` "
                    "after approximately %f seconds",
                    ev->id,
                    ti_name(),
                    diff);

            /* Reached time-out, kill the event */
            ++ti()->counters->events_killed;

            goto shift_drop_loop;
        }

process:
        assert (ev->status == TI_EVENT_STAT_READY);

        ti_event_log("processing", ev, LOGGER_DEBUG);

        if (ev->tp == TI_EVENT_TP_MASTER)
        {
            ti_query_run(ev->via.query);
        }
        else if (ti_event_run(ev) || ti_archive_push(ev->via.epkg))
        {
            /* logging is done, but we increment the failed counter and
             * log the full event */
            ++ti()->counters->events_failed;
            ti_event_log("event has failed", ev, LOGGER_ERROR);
        }

        /* update counters */
        (void) ti_counters_upd_commit_event(&ev->time);

        /* update committed event id */
        *cevid_p = ev->id;

        if (ev->flags & TI_EVENT_FLAG_SAVE)
            ti_save();

shift_drop_loop:
        (void) queue_shift(events->queue);
        ti_event_drop(ev);
    }

stop:
    uv_mutex_unlock(events->lock);

    /* status will be send to nodes on next `connect` loop */
    ti_connect_force_sync();

    /* trigger the event loop again (only if there are changes) */
    (void) events__trigger();
}

