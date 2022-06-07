/*
 * ti/changes.inline.h
 */
#ifndef TI_CHANGES_INLINE_H_
#define TI_CHANGES_INLINE_H_

#include <ti/thing.h>
#include <ti/changes.h>
#include <util/vec.h>

/*
 * The dropped list must take a reference because the thing might increase a
 * reference within a change, and then drop again. When that happens, the
 * thing will be added to the dropped list twice, so when destroying the
 * list we only need to destroy the last one.
 */
static inline _Bool ti_changes_cache_dropped_thing(ti_thing_t * thing)
{
    _Bool keep = changes_.keep_dropped && !vec_push(&changes_.dropped, thing);
    if (keep)
        ti_incref(thing);
    return keep;
}

#endif  /* TI_CHANGES_INLINE_H_*/
