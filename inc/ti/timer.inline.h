/*
 * ti/timer.inline.h
 */
#ifndef TI_TIMER_INLINE_H_
#define TI_TIMER_INLINE_H_

#include <ex.h>
#include <ti/timer.t.h>
#include <util/vec.h>

static inline ti_timer_t * ti_timer_by_name(vec_t * timers, ti_name_t * name)
{
    for (vec_each(timers, ti_timer_t, timer))
        if (timer->name == name)
            return timer;
    return NULL;
}

static inline void ti_timer_unsafe_drop(ti_timer_t * timer)
{
    if (!--timer->ref)
        ti_timer_destroy(timer);
}

#endif /* TI_TIMER_INLINE_H_ */
