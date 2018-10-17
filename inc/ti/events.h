/*
 * events.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef TI_EVENTS_H_
#define TI_EVENTS_H_

typedef struct ti_events_s ti_events_t;

#include <uv.h>
#include <stdint.h>
#include <ti/archive.h>
#include <util/vec.h>
#include <util/queue.h>

ti_events_t * ti_events_create(void);
void ti_events_destroy(ti_events_t * events);
int ti_events_init(ti_events_t * events);
void ti_events_close(ti_events_t * events);

/*
 * Changes to commit_id, obj_id and archive require the lock.
 */
struct ti_events_s
{
    uv_mutex_t lock;
    uint64_t commit_id;
    uint64_t next_id;
    queue_t * queue;         /* queued events */
    uint64_t archive_offset;
    ti_archive_t * archive;        /* archived (raw) events) */
    uv_async_t loop;
};


#endif /* TI_EVENTS_H_ */
