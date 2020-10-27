/*
 * ti/gc.h
 */
#ifndef TI_GC_H_
#define TI_GC_H_


typedef struct ti_gc_s ti_gc_t;

#include <inttypes.h>
#include <ti/thing.h>

ti_gc_t * ti_gc_create(uint64_t event_id, ti_thing_t * thing);

struct ti_gc_s
{
    uint64_t event_id;  /* committed event id during time of creation */
    ti_thing_t * thing; /* no reference, thing marked for garbage collection */
};

#endif  /* TI_GC_H_ */
