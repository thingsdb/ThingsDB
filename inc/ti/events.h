/*
 * ti/events.h
 */
#ifndef TI_EVENTS_H_
#define TI_EVENTS_H_

typedef struct ti_events_s ti_events_t;

extern ti_events_t events_;

#include <uv.h>
#include <stdint.h>
#include <ti/archive.h>
#include <ti/query.h>
#include <ti/thing.h>
#include <tiinc.h>
#include <util/queue.h>
#include <util/vec.h>

int ti_events_create(void);
int ti_events_start(void);
void ti_events_stop(void);
ssize_t ti_events_trigger_loop(void);
int ti_events_on_event(ti_node_t * from_node, ti_pkg_t * pkg);
int ti_events_create_new_event(ti_query_t * query, ex_t * e);
int ti_events_add_event(ti_node_t * node, ti_epkg_t * epkg);
_Bool ti_events_slave_req(ti_node_t * node, uint64_t event_id);
void ti_events_set_next_missing_id(uint64_t * event_id);
void ti_events_free_dropped(void);
int ti_events_resize_dropped(void);
static inline void ti_events_keep_dropped(void);
static inline _Bool ti_events_cache_dropped_thing(ti_thing_t * thing);

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
};

static inline void ti_events_keep_dropped(void)
{
    events_.keep_dropped = true;
}

static inline _Bool ti_events_cache_dropped_thing(ti_thing_t * thing)
{
    return events_.keep_dropped && !vec_push(&events_.dropped, thing);
}

#endif /* TI_EVENTS_H_ */
