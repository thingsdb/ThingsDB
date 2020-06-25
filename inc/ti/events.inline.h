/*
 * ti/events.inline.h
 */
#ifndef TI_EVENTS_INLINE_H_
#define TI_EVENTS_INLINE_H_

#include <ti/thing.h>
#include <util/vec.h>

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

#endif  /* TI_EVENTS_INLINE_H_*/
