/*
 * ti/thing.t.h
 *
 * Things in ThingsDB might store data in three different ways.
 *
 * 1. - Instance of a type, then the values are stored in a vector
 * and the keys are only stored on the type. The order of values must in this
 * case map that of the type.
 *
 * 2. - Object with strict keys, each key is compatible with the name
 * convention. In this case the properties are stored in a vector as key/value
 * pairs. The order is not important.
 *
 * 3. - Object might have keys which are not strict. Properties are stored in
 * a "smap" lookup as key/value pairs. The keys must be UTF-8 compatible and
 * no reserved keys (such as `#`) are allowed. When a key does follow the
 * name rules, the key must be of type `ti_name_t`, thus `ti_raw_t` keys are
 * never compatible with the naming convention.
 *
 * Specialized functions:
 *
 * - xxx_o_xxx : function is compatible with 2. and 3. only.
 * - xxx_t_xxx : function is compatible with 1. only.
 * - xxx_p_xxx : function is compatible with 2. only.
 * - xxx_i_xxx : function is compatible with 3. only.
 *
 * Without the "prefix" in the function name, the function should work on
 * a thing, no matter what type of thing it is.
 */
#ifndef TI_THING_T_H_
#define TI_THING_T_H_

typedef struct ti_thing_s  ti_thing_t;
typedef union ti_thing_via ti_thing_via_t;

#include <stdint.h>
#include <ti/collection.t.h>
#include <util/vec.h>

extern vec_t * ti_thing_gc_vec;

enum
{
    TI_THING_FLAG_SWEEP     =1<<0,      /* marked for sweep; each thing is
                                           initially marked and while running
                                           garbage collection this mark is
                                           removed as long as the `thing` is
                                           attached to the collection. */
    TI_THING_FLAG_NEW       =1<<1,      /* thing is new; new things require
                                           a full `dump` in a task while
                                           existing things only can contain
                                           the `id`.*/
    TI_THING_FLAG_DICT      =1<<2,      /* thing is an object and items are
                                           stored in the smap_t. */
};

union ti_thing_via
{
    vec_t * vec;                /* contains:
                                 *  - ti_prop_t :
                                 *          When thing is an object with
                                 *          only valid name properties.
                                 *  - ti_val_t :
                                 *          When thing is an instance of
                                 *               a type.
                                 */
    smap_t * smap;              /* contains ti_item_t :
                                 *          In an item the `key` value is of
                                 *          type ti_raw_t, but valid names are
                                 *          must still be of type ti_name_t
                                 *          because logic might decide a
                                 *          key does not exist in case there is
                                 *          no name available.
                                 */
};

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
    ti_thing_via_t items;           /* contains ti_prop_t, ti_item_t or
                                     * ti_val_t, depending if a thing is an
                                     * object strict,  with free keys, or
                                     * instance of a type */
    vec_t * watchers;               /* vec contains ti_watch_t,
                                       NULL if no watchers,  */
};


#endif /* TI_THING_T_H_ */
