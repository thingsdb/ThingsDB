/*
 * ti/timer.inline.h
 */
#ifndef TI_TIMER_INLINE_H_
#define TI_TIMER_INLINE_H_

#include <ex.h>
#include <ti/timer.t.h>
#include <ti/timer.h>
#include <util/vec.h>

static inline void ti_timer_drop(ti_timer_t * timer)
{
    if (timer && !--timer->ref)
        ti_timer_destroy(timer);
}

static inline void ti_timer_unsafe_drop(ti_timer_t * timer)
{
    if (!--timer->ref)
        ti_timer_destroy(timer);
}

static inline ti_timer_t * ti_timer_by_id(vec_t * timers, uint64_t id)
{
    for (vec_each(timers, ti_timer_t, timer))
        if (timer->id == id)
            return timer;
    return NULL;
}

#endif /* TI_TIMER_INLINE_H_ */
