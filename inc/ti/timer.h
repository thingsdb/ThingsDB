/*
 * ti/timer.h
 */
#ifndef TI_TIMER_H_
#define TI_TIMER_H_

#include <ex.h>
#include <ti/timer.t.h>

ti_timer_t * ti_timer_create(
        uint64_t id,
        ti_name_t * name,
        time_t next_run,
        uint32_t repeat,
        uint64_t scope_id,
        ti_user_t * user,
        ti_closure_t * closure,
        vec_t * args);
void ti_timer_destroy(ti_timer_t * timer);
void ti_timer_run(ti_timer_t * timer);
void ti_timer_fwd(ti_timer_t * timer);
void ti_timer_broadcast_e(ti_timer_t * timer);
void ti_timer_ex_set(
        ti_timer_t * timer,
        ex_enum errnr,
        const char * errmsg,
        ...);
void ti_timer_ex_cpy(ti_timer_t * timer, ex_t * e);
ti_timer_t * ti_timer_by_name(vec_t * timers, ti_name_t * name);

#endif /* TI_TIMER_H_ */
