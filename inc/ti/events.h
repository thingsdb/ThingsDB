/*
 * ti/events.h
 */
#ifndef TI_EVENTS_H_
#define TI_EVENTS_H_

#include <ti/events.t.h>
#include <ti/node.t.h>
#include <ti/proto.t.h>
#include <ti/query.t.h>
#include <ti/thing.t.h>
#include <util/vec.h>

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

static inline void ti_events_keep_dropped(void)
{
    events_.keep_dropped = true;
}

static inline _Bool ti_events_in_queue(void)
{
    return events_.queue->n != 0;
}

#endif /* TI_EVENTS_H_ */
