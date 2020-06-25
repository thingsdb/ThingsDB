/*
 * ti/events.t.h
 */
#ifndef TI_EVENTS_T_H_
#define TI_EVENTS_T_H_

typedef struct ti_events_s ti_events_t;

extern ti_events_t events_;

#include <inttypes.h>
#include <util/olist.h>
#include <util/queue.h>
#include <util/util.h>
#include <util/vec.h>
#include <uv.h>

/*
 * Changes to commit_id, archive require the lock.
 */
struct ti_events_s
{
    _Bool is_started;
    _Bool keep_dropped;
    uv_mutex_t * lock;
    uint64_t next_event_id;     /* next event id, starts at 1 */
    queue_t * queue;            /* queued events (ti_event_t), usually an
                                   event has only one reference which is
                                   hold by this queue. (order low->high) */
    uv_async_t * evloop;
    vec_t * dropped;            /* ti_thing_t, dropped while running event */
    olist_t * skipped_ids;
    util_time_t wait_gap_time;
    uint64_t wait_cevid;
};

#endif /* TI_EVENTS_T_H_ */
