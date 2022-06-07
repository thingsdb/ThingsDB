/*
 * ti/change.t.h
 */
#ifndef TI_CHANGES_T_H_
#define TI_CHANGES_T_H_

typedef struct ti_changes_s ti_changes_t;

extern ti_changes_t changes_;

#include <inttypes.h>
#include <util/olist.h>
#include <util/queue.h>
#include <util/util.h>
#include <util/vec.h>
#include <uv.h>

/*
 * Changes to commit_id, archive require the lock.
 */
struct ti_changes_s
{
    _Bool is_started;
    _Bool keep_dropped;
    uv_mutex_t * lock;
    uint64_t next_change_id;    /* next change id, starts at 1 */
    queue_t * queue;            /* queued changes (ti_change_t), usually a
                                   change has only one reference which is
                                   hold by this queue. (order low->high) */
    uv_async_t * changeloop;
    vec_t * dropped;            /* ti_thing_t, dropped while running change */
    olist_t * skipped_ids;
    util_time_t wait_gap_time;
    uint64_t wait_ccid;
};

#endif /* TI_CHANGES_T_H_ */
