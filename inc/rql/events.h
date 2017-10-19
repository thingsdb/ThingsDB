/*
 * events.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_EVENTS_H_
#define RQL_EVENTS_H_

typedef struct rql_events_s rql_events_t;

#include <uv.h>
#include <inttypes.h>
#include <rql/rql.h>
#include <util/vec.h>
#include <util/queue.h>

rql_events_t * rql_events_create(rql_t * rql);
void rql_events_destroy(rql_events_t * events);
int rql_events_init(rql_events_t * events);

int rql_events_store(rql_events_t * events, const char * fn);
int rql_events_restore(rql_events_t * events, const char * fn);

static inline uint64_t rql_events_get_obj_id(rql_events_t * events);

struct rql_events_s
{
    rql_t * rql;
    uint64_t commit_id;
    uint64_t next_id;
    uint64_t obj_id;         /* used for assigning id to user or database */
    queue_t * queue;         /* queued events */
    vec_t * done;
    uv_async_t loop;
};

static inline uint64_t rql_events_get_obj_id(rql_events_t * events)
{
    return events->obj_id++;
}

#endif /* RQL_EVENTS_H_ */
