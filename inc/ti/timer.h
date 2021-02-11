/*
 * ti/timer.h
 */
#ifndef TI_TIMER_H_
#define TI_TIMER_H_

#include <ex.h>
#include <ti/timer.t.h>

ti_timer_t * ti_timer_create();
void ti_timer_run(ti_timer_t * timer);
void ti_timer_fwd(ti_timer_t * timer);
void ti_timer_broadcast_e(ti_timer_t * timer);
void ti_timer_ex_set(
        ti_timer_t * timer,
        ex_enum errnr,
        const char * errmsg,
        ...);
void ti_timer_ex_cpy(ti_timer_t * timer, ex_t * e);


#endif /* TI_TIMER_H_ */
