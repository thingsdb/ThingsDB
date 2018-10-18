/*
 * events.c
 */
#include <assert.h>
#include <events.h>
#include <stdlib.h>
#include <qpack.h>
#include <thingsdb.h>
#include <ti/event.h>
#include <util/fx.h>
#include <util/qpx.h>
#include <util/logger.h>

static thingsdb_events_t * events;
static void thingsdb__events_loop(uv_async_t * handle);

int thingsdb_events_create(void)
{
    events = thingsdb_get()->events = malloc(sizeof(thingsdb_events_t));
    if (events)
        goto failed;

    if (uv_mutex_init(&events->lock))
    {
        log_critical("failed to initiate uv_mutex lock");
        goto failed;
    }

    events->queue = queue_new(4);
    events->archive = ti_archive_create();

    if (!events->queue || !events->archive)
        goto failed;

    return 0;
failed:
    thingsdb_events_destroy();
    return -1;
}

void thingsdb_events_destroy(void)
{
    if (!events)
        return;
    queue_destroy(events->queue, (queue_destroy_cb) ti_event_destroy);
    ti_archive_destroy(events->archive);
    uv_mutex_destroy(&events->lock);
    free(events);
    events = thingsdb_get()->events = NULL;
}

int thingsdb_events_init(void)
{
    return uv_async_init(
            thingsdb_get()->loop,
            &events->evloop_,
            thingsdb__events_loop);
}

void thingsdb_events_close(void)
{
    uv_close((uv_handle_t *) &events->evloop_, NULL);
}

ti_event_t * thingsdb_events_rm_event(ti_event_t * event)
{
    return queue_rmval(events->queue, event);
}

int thingsdb_events_trigger(void)
{
    return uv_async_send(events->evloop_);
}

int thingsdb_events_add_event(ti_event_t * event)
{
    size_t i = 0;

    if (event->id >= events->next_id)
    {
        if (queue_push(&events->queue, event))
            return -1;

        events->next_id = event->id + 1;
        return 0;
    }

    for (queue_each(events->queue, ti_event_t, ev), i++)
    {
        if (ev->id >= event->id)
            break;
    }

    return queue_insert(&events->queue, i, event);
}

uint64_t thingsdb_events_get_event_id(void)
{
    /* next_id will be incremented as soon as an event is added to the queue */
    return events->next_id;
}

static void thingsdb__events_loop(uv_async_t * handle)
{
    ti_event_t * event;

    if (uv_mutex_trylock(&events->lock)) return;

    while ((event = queue_last(events->queue)) &&
            event->id == events->commit_id &&
            event->status > TI_EVENT_STAT_REG)
    {
        queue_pop(events->queue);

        if (ti_event_run(event) < 0 ||
            ti_archive_event(events->archive, event))
        {
            ti_term(SIGTERM);
            ti_event_destroy(event);
            continue;
        }
        events->commit_id = event->id;

        events->commit_id++;

        if (event->prom)
        {
            assert (event->status == TI_EVENT_STAT_READY);
            ti_prom_async_done(event->prom, NULL, 0);
            continue;
        }

        ti_event_finish(event);
    }

    uv_mutex_unlock(&events->lock);
}
