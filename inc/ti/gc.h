/*
 * ti/gc.h
 */
#ifndef TI_GC_H_
#define TI_GC_H_


typedef struct ti_gc_s ti_gc_t;

#include <inttypes.h>
#include <ti/thing.h>

ti_gc_t * ti_gc_create(uint64_t event_id, ti_thing_t * thing);
void ti_gc_destroy(ti_gc_t * gc);

struct ti_gc_s
{
    uint64_t event_id;  /* committed event id during time of creation */
    ti_thing_t * thing; /* with reference, thing marked for garbage collection */
};

static inline int ti_gc_walk(queue_t * queue, queue_cb cb, void * arg)
{
    int rc;
    for (queue_each(queue, ti_gc_t, gc))
        if ((rc = cb(gc->thing, arg)))
            return rc;
    return 0;
}

#endif  /* TI_GC_H_ */
