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
#include <ti/proto.h>
#include <tiinc.h>
#include <util/queue.h>
#include <util/vec.h>
#include <util/olist.h>
#include <util/util.h>

int ti_events_create(void);
int ti_events_start(void);
void ti_events_stop(void);
ssize_t ti_events_trigger_loop(void);
int ti_events_on_event(ti_node_t * from_node, ti_pkg_t * pkg);
int ti_events_create_new_event(ti_query_t * query, ex_t * e);
int ti_events_add_event(ti_node_t * node, ti_epkg_t * epkg);
ti_proto_enum_t ti_events_accept_id(uint64_t event_id, uint8_t * n);
void ti_events_set_next_missing_id(uint64_t * event_id);
void ti_events_free_dropped(void);
int ti_events_resize_dropped(void);
vec_t * ti_events_pkgs_from_queue(ti_thing_t * thing);

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

static inline void ti_events_keep_dropped(void)
{
    events_.keep_dropped = true;
}

/*
 * The dropped list must take a reference because the thing might increase a
 * reference within an event, and then drop again. When that happens, the
 * thing will be added to the dropped list twice, so when destroying the
 * list we only need to destroy the last one.
 */
static inline _Bool ti_events_cache_dropped_thing(ti_thing_t * thing)
{
    _Bool keep = events_.keep_dropped && !vec_push(&events_.dropped, thing);
    if (keep)
        ti_incref(thing);
    return keep;
}

static inline _Bool ti_events_is_empty(void)
{
    return events_.queue->n == 0;
}

#endif /* TI_EVENTS_H_ */
