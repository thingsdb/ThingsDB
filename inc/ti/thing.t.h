/*
 * ti/thing.t.h
 */
#ifndef TI_THING_T_H_
#define TI_THING_T_H_

typedef struct ti_thing_s  ti_thing_t;

#include <stdint.h>
#include <ti/collection.t.h>
#include <util/vec.h>

extern vec_t * ti_thing_gc_vec;

struct ti_thing_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    uint16_t type_id;

    uint64_t id;
    ti_collection_t * collection;   /* thing belongs to this collection;
                                     * only `null` when in thingsdb or node
                                     * scope, but never in a collection scope
                                     */
    vec_t * items;                  /* vec contains ti_prop_t or ti_val_t,
                                     * depending if a thing is an object or
                                     * instance */
    vec_t * watchers;               /* vec contains ti_watch_t,
                                       NULL if no watchers,  */
};


#endif /* TI_THING_T_H_ */
