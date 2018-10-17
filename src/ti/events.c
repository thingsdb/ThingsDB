/*
 * events.c
 */
#include <assert.h>
#include <stdlib.h>
#include <qpack.h>
#include <thingsdb.h>
#include <ti/event.h>
#include <ti/events.h>
#include <util/fx.h>
#include <util/qpx.h>
#include <util/logger.h>

static void ti__events_loop(uv_async_t * handle);

ti_events_t * ti_events_create(void)
{
    ti_events_t * events = malloc(sizeof(ti_events_t));
    if (!events)
        return NULL;

    if (uv_mutex_init(&events->lock))
    {
        log_critical("failed to initiate uv_mutex lock");
        free(events);
        return NULL;
    }

    events->queue = queue_new(4);
    events->archive = ti_archive_create();
    events->loop.data = events;

    return events;
}

void ti_events_destroy(ti_events_t * events)
{
    if (!events) return;
    queue_destroy(events->queue, (queue_destroy_cb) ti_event_destroy);
    ti_archive_destroy(events->archive);
    uv_mutex_destroy(&events->lock);
    free(events);
}

int ti_events_init(ti_events_t * events)
{
    return uv_async_init(thingsdb_loop(), &events->loop, ti__events_loop);
}

void ti_events_close(ti_events_t * events)
{
    uv_close((uv_handle_t *) &events->loop, NULL);
}

static void ti__events_loop(uv_async_t * handle)
{
    ti_events_t * events = (ti_events_t *) handle->data;
    ti_event_t * event;

    if (uv_mutex_trylock(&events->lock)) return;

    while ((event = queue_last(events->queue)) &&
            event->id == events->commit_id &&
            event->status > TI_EVENT_STAT_REG)
    {
        queue_pop(event->events->queue);

        if (ti_event_run(event) < 0 ||
            ti_archive_event(events->archive, event))
        {
            ti_term(SIGTERM);
            ti_event_destroy(event);
            continue;
        }
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
