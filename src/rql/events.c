/*
 * events.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <assert.h>
#include <stdlib.h>
#include <qpack.h>
#include <rql/events.h>
#include <rql/event.h>
#include <util/fx.h>
#include <util/qpx.h>
#include <util/logger.h>

static void rql__events_loop(uv_async_t * handle);

rql_events_t * rql_events_create(rql_t * rql)
{
    rql_events_t * events = malloc(sizeof(rql_events_t));
    if (!events)
        return NULL;

    if (uv_mutex_init(&events->lock))
    {
        log_critical("failed to initiate uv_mutex lock");
        free(events);
        return NULL;
    }

    events->rql = rql;
    events->queue = queue_new(4);
    events->archive = rql_archive_create();
    events->loop.data = events;

    return events;
}

void rql_events_destroy(rql_events_t * events)
{
    if (!events) return;
    queue_destroy(events->queue, (queue_destroy_cb) rql_event_destroy);
    rql_archive_destroy(events->archive);
    uv_mutex_destroy(&events->lock);
    free(events);
}

int rql_events_init(rql_events_t * events)
{
    return uv_async_init(&events->rql->loop, &events->loop, rql__events_loop);
}

void rql_events_close(rql_events_t * events)
{
    uv_close((uv_handle_t *) &events->loop, NULL);
}

static void rql__events_loop(uv_async_t * handle)
{
    rql_events_t * events = (rql_events_t *) handle->data;
    rql_event_t * event;

    if (uv_mutex_trylock(&events->lock)) return;

    while ((event = queue_last(events->queue)) &&
            event->id == events->commit_id &&
            event->status > RQL_EVENT_STAT_REG)
    {
        queue_pop(event->events->queue);

        if (rql_event_run(event) < 0 ||
            rql_archive_event(events->archive, event))
        {
            rql_term(SIGTERM);
            rql_event_destroy(event);
            continue;
        }
        events->commit_id++;

        if (event->prom)
        {
            assert (event->status == RQL_EVENT_STAT_READY);
            rql_prom_async_done(event->prom, NULL, 0);
            continue;
        }

        rql_event_finish(event);
    }

    uv_mutex_unlock(&events->lock);
}
