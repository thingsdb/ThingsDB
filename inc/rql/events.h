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
#include <rql/archive.h>
#include <util/vec.h>
#include <util/queue.h>

rql_events_t * rql_events_create(rql_t * rql);
void rql_events_destroy(rql_events_t * events);
int rql_events_init(rql_events_t * events);
void rql_events_close(rql_events_t * events);

/*
 * Changes to commit_id, obj_id and archive require the lock.
 */
struct rql_events_s
{
    uv_mutex_t lock;
    rql_t * rql;
    uint64_t commit_id;
    uint64_t next_id;
    queue_t * queue;         /* queued events */
    uint64_t archive_offset;
    rql_archive_t * archive;        /* archived (raw) events) */
    uv_async_t loop;
};


#endif /* RQL_EVENTS_H_ */
