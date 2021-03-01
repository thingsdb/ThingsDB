/*
 * ti/timer.h
 */
#ifndef TI_TIMER_H_
#define TI_TIMER_H_

#include <ex.h>
#include <ti/raw.t.h>
#include <ti/timer.t.h>
#include <ti/val.t.h>

ti_timer_t * ti_timer_create(
        uint64_t id,
        uint64_t next_run,
        uint32_t repeat,
        uint64_t scope_id,
        ti_user_t * user,
        ti_closure_t * closure,
        vec_t * args);
void ti_timer_destroy(ti_timer_t * timer);
void ti_timer_run(ti_timer_t * timer);
void ti_timer_fwd(ti_timer_t * timer);
void ti_timer_mark_del(ti_timer_t * timer);
ti_raw_t * ti_timer_doc(ti_timer_t * timer);
ti_raw_t * ti_timer_def(ti_timer_t * timer);
void ti_timer_done(ti_timer_t * timer, ex_t * e);
ti_timer_t * ti_timer_from_val(vec_t * timers, ti_val_t * val, ex_t * e);
ti_val_t * ti_timer_as_mpval(ti_timer_t * timer, _Bool with_full_access);

#endif /* TI_TIMER_H_ */
