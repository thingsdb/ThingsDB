/*
 * events.h
 */
#ifndef THINGSDB_EVENTS_H_
#define THINGSDB_EVENTS_H_

typedef struct thingsdb_events_s thingsdb_events_t;

#include <uv.h>
#include <stdint.h>
#include <ti/archive.h>
#include <util/vec.h>
#include <util/queue.h>

int thingsdb_events_create(void);
void thingsdb_events_destroy(void);
int thingsdb_events_init(void);
void thingsdb_events_close(void);
ti_event_t * thingsdb_events_rm_event(ti_event_t * event);
int thingsdb_events_trigger(void);
int thingsdb_events_add_event(ti_event_t * event);
uint64_t thingsdb_events_get_event_id(void);

/*
 * Changes to commit_id, archive require the lock.
 */
struct thingsdb_events_s
{
    uv_mutex_t lock;
    uint64_t commit_id;      /* last event id which s commited */
    uint64_t next_id;        /* next event id */
    queue_t * queue;         /* queued events */
    uint64_t archive_offset;
    ti_archive_t * archive;  /* archived (raw) events) */
    uv_async_t evloop_;
};


#endif /* THINGSDB_EVENTS_H_ */
