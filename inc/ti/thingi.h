/*
 * ti/thingi.h
 */
#ifndef TI_THINGI_H_
#define TI_THINGI_H_

#include <ti/type.h>
#include <ti/thing.h>

/* returns IMAP_ERR_EXIST if the thing is already in the map */
static inline int ti_thing_to_map(ti_thing_t * thing)
{
    return imap_add(thing->collection->things, thing->id, thing);
}

static inline ti_type_t * ti_thing_type(ti_thing_t * thing)
{
    ti_type_t * type = imap_get(thing->collection->types->imap, thing->type_id);
    assert (type);
    return type;
}

static inline const char * ti_thing_type_str(ti_thing_t * thing)
{
    return ti_thing_type(thing)->name;
}

#endif  /* TI_THINGI_H_ */
