/*
 * events.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <qpack.h>
#include <rql/events.h>
#include <rql/event.h>

static void rql__events_loop(uv_async_t * handle);

rql_events_t * rql_events_create(rql_t * rql)
{
    rql_events_t * events = (rql_events_t *) malloc(sizeof(rql_events_t));
    if (!events) return NULL;
    events->rql = rql;
    events->queue = queue_new(4);
    events->done = vec_new(4);
    return events;
}

void rql_events_destroy(rql_events_t * events)
{
    if (!events) return;
    queue_destroy(events->queue, (queue_destroy_cb) rql_event_destroy);
    vec_destroy(events->done, free);
    free(events);
}

int rql_events_init(rql_events_t * events)
{
    return uv_async_init(&events->rql->loop, &events->loop, rql__events_loop);
}

static void rql__events_loop(uv_async_t * handle)
{
    rql_events_t * events = (rql_events_t *) handle->data;

    while (events->queue->n)
    {
        rql_event_t * event = (rql_event_t *) queue_get(
                events->queue,
                events->queue->n - 1);
        if (event->id == events->commit_id)
        {
            rql_event_run(event);
            queue_pop(events->queue);
            rql_event_done(event);
            rql_event_destroy(event);
        }
    }
}
