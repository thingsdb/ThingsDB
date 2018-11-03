/*
 * events.c
 */
#include <assert.h>
#include <qpack.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/event.h>
#include <ti/events.h>
#include <ti/prom.h>
#include <ti/proto.h>
#include <util/fx.h>
#include <util/logger.h>
#include <util/qpx.h>
#include <util/vec.h>

static ti_events_t * events;

static void events__destroy(uv_handle_t * UNUSED(handle));
static void events__new_id(ti_event_t * ev);
static int events__req_event_id(ti_event_t * ev, ex_t * e);
static void events__on_req_event_id(ti_prom_t * prom);
static void events__loop(uv_async_t * handle);
static inline int events__trigger(void);

int ti_events_create(void)
{
    events = ti()->events = malloc(sizeof(ti_events_t));
    if (!events)
        goto failed;

    if (uv_mutex_init(&events->lock))
    {
        log_critical("failed to initiate uv_mutex lock");
        goto failed;
    }

    events->is_started = false;
    events->queue = queue_new(4);
    events->archive = ti_archive_create();
    events->evloop = malloc(sizeof(uv_async_t));

    if (!events->queue || !events->archive)
        goto failed;

    return 0;

failed:
    ti_events_stop();
    return -1;
}

int ti_events_start(void)
{
    if (uv_async_init(ti()->loop, events->evloop, events__loop))
        return -1;
    events->is_started = true;
    return 0;
}

void ti_events_stop(void)
{
    if (!events)
        return;

    if (events->is_started)
        uv_close((uv_handle_t *) events->evloop, events__destroy);
    else
        events__destroy(NULL);
}

int ti_events_create_new_event(ti_query_t * query, ex_t * e)
{
    ti_event_t * ev;

    if (!ti_nodes_has_quorum())
    {
        ex_set(e, EX_NODE_ERROR,
                "node `%s` does not have the required quorum "
                "of at least %u connected nodes",
                ti_node_name(ti()->node),
                ti_nodes_quorum());
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
    query->ev = ev;
    ev->target = ti_grab(query->target);

    return events__req_event_id(ev, e);
}

_Bool ti_events_check_id(ti_node_t * node, uint64_t event_id)
{
    ti_event_t * ev;
    ti_node_t * prev_node;

    if (event_id == events->next_event_id)
        return true;

    if (event_id > events->next_event_id)
    {
        log_debug(
                "next expected event id is `%"PRIu64"` but received "
                "id `%"PRIu64"`", events->next_event_id, event_id);
        return true;
    }

    ev = queue_last(events->queue);

    if (ev->id != event_id)
        return false;

    prev_node = ev->tp == TI_EVENT_TP_MASTER ? ti()->node : ev->via.node;

    if (ti_node_winner(node, prev_node, event_id) == prev_node)
        return false;

    if (ev->tp == TI_EVENT_TP_MASTER)
        events__new_id(ev);
    else
    {
        (void *) queue_pop(events->queue);
        ti_event_destroy(ev);
    }

    return true;
}

static void events__destroy(uv_handle_t * UNUSED(handle))
{
    if (!events)
        return;
    queue_destroy(events->queue, (queue_destroy_cb) ti_event_destroy);
    ti_archive_destroy(events->archive);
    uv_mutex_destroy(&events->lock);
    free(events->evloop);
    free(events);
    events = ti()->events = NULL;
}

static void events__new_id(ti_event_t * ev)
{
    ex_t * e = ex_use();

    /* we probably get here twice, when the request is not accepted and when
     * we receive "the" another conflicting event id
     */
    if (!queue_rmval(events->queue, ev))
        return;

    if (events__req_event_id(ev, e))
    {
        ti_query_send(ev->via.query, e);
        ti_event_cancel(ev);
    }
}

static int events__req_event_id(ti_event_t * ev, ex_t * e)
{
    assert (queue_space(events->queue) > 0);

    vec_t * vec_nodes = ti()->nodes->vec;
    ti_prom_t * prom;
    qpx_packer_t * packer;
    ti_pkg_t * pkg;

    prom = ti_prom_new(vec_nodes->n - 1, events__on_req_event_id, ev);
    if (!prom)
    {
        ex_set_alloc(e);
        return e->nr;
    }

    packer = qpx_packer_create(9, 0);
    if (!packer)
    {
        ti_prom_destroy(prom);
        ex_set_alloc(e);
        return e->nr;
    }

    ev->id = events->next_event_id;
    ++events->next_event_id;

    (void) qp_add_int64(packer, ev->id);
    pkg = qpx_packer_pkg(packer, TI_PROTO_NODE_REQ_EVENT_ID);

    QUEUE_push(events->queue, ev);

    for (vec_each(vec_nodes, ti_node_t, node))
    {
        if (node == ti()->node)
            continue;

        if (node->status <= TI_NODE_STAT_CONNECTING || ti_req_create(
                node->stream,
                pkg,
                TI_PROTO_NODE_REQ_EVENT_ID_TIMEOUT,
                ti_prom_req_cb,
                prom))
            prom->sz--;
    }

    if (!prom->sz)
        free(pkg);

    ti_prom_go(prom);

    return 0;
}

static void events__on_req_event_id(ti_prom_t * prom)
{
    assert (prom->n == prom->sz);

    _Bool accepted = true;
    ti_event_t * ev = prom->data;
    ex_enum exnr = EX_SUCCESS;

    for (size_t i = 0; i < prom->n; ++i)
    {
        ti_prom_res_t * res = &prom->res[i];
        ti_req_t * req = res->via.req;

        assert (res->tp == TI_PROM_VIA_REQ);

        if (res->status)
        {
            ev->status = TI_EVENT_STAT_CACNCEL;
            exnr = res->status;
        }
        else if (req->pkg_res->tp != TI_PROTO_NODE_RES_EVENT_ID)
        {
            accepted = false;
        }

        ti_req_destroy(req);
    }

    if (ev->status == TI_EVENT_STAT_CACNCEL)
    {
        ex_t * e = ex_use();
        ex_set(e, EX_NODE_ERROR, ex_str(exnr));
        ti_query_send(ev->via.query, e);
        ti_event_cancel(ev);
        goto done;
    }

    if (!accepted)
    {
        events__new_id(ev);
        goto done;
    }

    ev->status = TI_EVENT_STAT_READY;

    if (events__trigger())
    {
        log_error("cannot trigger the events loop");
        /* do nothing, it could be triggered later */
    }

done:
    ti_prom_destroy(prom);
}

static void events__loop(uv_async_t * UNUSED(handle))
{
    ti_event_t * ev;

    if (uv_mutex_trylock(&events->lock))
        return;

    while ((ev = queue_last(events->queue)) &&
            ev->id == (events->commit_event_id + 1) &&
            ev->status > TI_EVENT_STAT_NEW)
    {
        (void *) queue_pop(events->queue);

        if (ev->status == TI_EVENT_STAT_CACNCEL)
        {
            /* nothing */
        }
        else if (ev->tp == TI_EVENT_TP_MASTER)
        {
            assert (ev->status == TI_EVENT_STAT_READY);

            ti_query_run(ev->via.query);
        }
        else
        {
            assert (ev->tp == TI_EVENT_TP_SLAVE);
            assert (ev->status == TI_EVENT_STAT_READY);
            /* TODO: run event tasks */
        }

        events->commit_event_id = ev->id;
        ti_event_destroy(ev);
    }

    uv_mutex_unlock(&events->lock);
}

static inline int events__trigger(void)
{
    return uv_async_send(events->evloop);
}

//{
//    ti_event_t * event = (ti_event_t *) prom->data;
//    free(event->req_pkg);
//    _Bool accept = true;
//
//    for (size_t i = 0; i < prom->n; i++)
//    {
//
//        ti_prom_res_t * res = &prom->res[i];
//        ti_req_t * req = (ti_req_t *) res->handle;
//
//        if (res->status)
//        {
//            event->status = TI_EVENT_STAT_CACNCEL;
//        }
//        else if (req->pkg_res->tp == TI_PROTO_REJECT)
//        {
//            accept = false;
//        }
//
//        ti_req_destroy(req);
//    }
//
//    free(prom);
//
//    if (event->status == TI_EVENT_STAT_CACNCEL)
//    {
//        // remove from queue if it is still in the queue.
//        // event->status could be already set to reject in which case the
//        // event is already removed from the queue
//        ti__event_cancel(event);  /* terminates in case of failure */
//        ti_events_trigger();
//        return;
//    }
//
//    if (!accept)
//    {
//        uint64_t old_id = event->id;
//        (void *) ti_events_rm_event(event);
//        event->id = ti_events_get_event_id();
//        (void) ti_events_add_event(event);  /* queue has room */
//        ti__event_upd(event, old_id);  /* terminates in case of failure */
//        return;
//    }
//
//    ti__event_ready(event);  /* terminates in case of failure */
//}


//
//void ti_events_close(void)
//{
//    uv_close((uv_handle_t *) &events->evloop_, NULL);
//}

//ti_event_t * ti_events_rm_event(ti_event_t * event)
//{
//    return queue_rmval(events->queue, event);
//}
//
//int ti_events_trigger(void)
//{
//    return uv_async_send(&events->evloop_);
//}

//int ti_events_add_event(ti_event_t * event)
//{
//    size_t i = 0;
//
//    if (event->id >= events->next_event_id)
//    {
//        if (queue_push(&events->queue, event))
//            return -1;
//
//        events->next_event_id = event->id + 1;
//        return 0;
//    }
//
//    for (queue_each(events->queue, ti_event_t, ev), i++)
//    {
//        if (ev->id >= event->id)
//            break;
//    }
//
//    return queue_insert(&events->queue, i, event);
//}
//
//uint64_t ti_events_get_event_id(void)
//{
//    /* next_id will be incremented as soon as an event is added to the queue */
//    return events->next_event_id;
//}

//static void ti__events_loop(uv_async_t * handle)
//{
//    ti_event_t * event;
//
//    if (uv_mutex_trylock(&events->lock))
//        return;
//
//    while ((event = queue_last(events->queue)) &&
//            event->id == events->commit_event_id &&
//            event->status > TI_EVENT_STAT_REG)
//    {
//        queue_pop(events->queue);
//
//        if (ti_event_run(event) < 0 ||
//            ti_archive_event(events->archive, event))
//        {
//            ti_term(SIGTERM);
//            ti_event_destroy(event);
//            continue;
//        }
//        events->commit_event_id = event->id;
//
//        events->commit_event_id++;
//
//        if (event->prom)
//        {
//            assert (event->status == TI_EVENT_STAT_READY);
//            ti_prom_async_done(event->prom, NULL, 0);
//            continue;
//        }
//
//        ti_event_finish(event);
//    }
//
//    uv_mutex_unlock(&events->lock);
//}
