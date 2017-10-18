/*
 * events.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <qpack.h>
#include <rql/events.h>

rql_events_t * rql_events_create(rql_t * rql)
{
    rql_events_t * events = (rql_events_t *) malloc(sizeof(rql_events_t));
    if (!events) return NULL;

    return events;
}

void rql_events_destroy(rql_events_t * events)
{
    if (!events) return;
    free(events);
}
