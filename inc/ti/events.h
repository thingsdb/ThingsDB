/*
 * events.h
 */
#ifndef TI_EVENTS_H_
#define TI_EVENTS_H_

typedef struct ti_events_s ti_events_t;

#include <uv.h>
#include <stdint.h>
#include <ti/archive.h>
#include <util/vec.h>
#include <util/queue.h>

int ti_events_create(void);
void ti_events_destroy(void);
int ti_events_init(void);
void ti_events_close(void);
ti_event_t * ti_events_rm_event(ti_event_t * event);
int ti_events_trigger(void);
int ti_events_add_event(ti_event_t * event);
uint64_t ti_events_get_event_id(void);

/*
 * Changes to commit_id, archive require the lock.
 */
struct ti_events_s
{
    uv_mutex_t lock;
    uint64_t commit_event_id;      /* last event id which is committed or 0
                                      if not event is committed yet */
    uint64_t next_event_id;        /* next event id, starts at 1 */
    queue_t * queue;            /* queued events */
    uint64_t archive_offset;
    ti_archive_t * archive;  /* archived (raw) events) */
    uv_async_t evloop_;
};


#endif /* TI_EVENTS_H_ */
