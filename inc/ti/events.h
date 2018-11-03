/*
 * events.h
 */
#ifndef TI_EVENTS_H_
#define TI_EVENTS_H_

typedef struct ti_events_s ti_events_t;

#include <uv.h>
#include <stdint.h>
#include <ti/archive.h>
#include <ti/query.h>
#include <util/vec.h>
#include <util/queue.h>

int ti_events_create(void);
int ti_events_start(void);
void ti_events_stop(void);
int ti_events_create_new_event(ti_query_t * query, ex_t * e);

/*
 * Changes to commit_id, archive require the lock.
 */
struct ti_events_s
{
    _Bool is_started;
    uv_mutex_t lock;
    uint64_t commit_event_id;      /* last event id which is committed or 0
                                      if no event is committed yet */
    uint64_t next_event_id;        /* next event id, starts at 1 */
    queue_t * queue;            /* queued events */
    ti_archive_t * archive;  /* archived (raw) events) */
    uv_async_t * evloop;
};


#endif /* TI_EVENTS_H_ */
