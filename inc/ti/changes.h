/*
 * ti/changes.h
 */
#ifndef TI_CHANGES_H_
#define TI_CHANGES_H_

#include <ti/changes.t.h>
#include <ti/node.t.h>
#include <ti/proto.t.h>
#include <ti/query.t.h>
#include <ti/thing.t.h>
#include <util/vec.h>

int ti_changes_create(void);
int ti_changes_start(void);
void ti_changes_stop(void);
ssize_t ti_changes_trigger_loop(void);
int ti_changes_on_change(ti_node_t * from_node, ti_pkg_t * pkg);
int ti_changes_create_new_change(ti_query_t * query, ex_t * e);
int ti_changes_add_change(ti_node_t * node, ti_cpkg_t * cpkg);
ti_proto_enum_t ti_changes_accept_id(uint64_t change_id, uint8_t * n);
void ti_changes_set_next_missing_id(uint64_t * change_id);
void ti_changes_free_dropped(void);
int ti_changes_resize_dropped(void);

static inline void ti_changes_keep_dropped(void)
{
    changes_.keep_dropped = true;
}

static inline _Bool ti_changes_in_queue(void)
{
    return changes_.queue->n != 0;
}

#endif /* TI_CHANGES_H_ */
