/*
 * ti/thing.h
 */
#ifndef TI_THING_H_
#define TI_THING_H_

typedef struct ti_thing_s  ti_thing_t;

#include <assert.h>
#include <stdint.h>
#include <ti/collection.h>
#include <ti/field.h>
#include <ti/name.h>
#include <ti/prop.h>
#include <ti/raw.h>
#include <ti/spec.h>
#include <ti/stream.h>
#include <ti/type.h>
#include <ti/val.h>
#include <ti/watch.h>
#include <ti/wprop.h>
#include <util/imap.h>
#include <util/mpack.h>
#include <util/vec.h>

ti_thing_t * ti_thing_o_create(uint64_t id, ti_collection_t * collection);
ti_thing_t * ti_thing_t_create(
        uint64_t id,
        ti_type_t * type,
        ti_collection_t * collection);
void ti_thing_destroy(ti_thing_t * thing);
void ti_thing_clear(ti_thing_t * thing);
int ti_thing_props_from_unp(
        ti_thing_t * thing,
        ti_val_unp_t * vup,
        size_t sz,
        ex_t * e);
ti_thing_t * ti_thing_new_from_unp(ti_val_unp_t * vup, size_t sz, ex_t * e);
ti_prop_t * ti_thing_o_prop_add(    /* only when property does not exists */
        ti_thing_t * thing,
        ti_name_t * name,
        ti_val_t * val);
ti_prop_t * ti_thing_o_prop_set(
        ti_thing_t * thing,
        ti_name_t * name,
        ti_val_t * val);
void ti_thing_t_prop_set(ti_thing_t * thing, ti_name_t * name, ti_val_t * val);
void ti_thing_t_to_object(ti_thing_t * thing);
_Bool ti_thing_o_del(ti_thing_t * thing, ti_name_t * name);
int ti_thing_o_del_e(ti_thing_t * thing, ti_raw_t * rname, ex_t * e);
_Bool ti_thing_get_by_raw(ti_wprop_t * wprop, ti_thing_t * thing, ti_raw_t * raw);
int ti_thing_get_by_raw_e(
        ti_wprop_t * wprop,
        ti_thing_t * thing,
        ti_raw_t * r,
        ex_t * e);
int ti_thing_gen_id(ti_thing_t * thing);
ti_watch_t * ti_thing_watch(ti_thing_t * thing, ti_stream_t * stream);
_Bool ti_thing_unwatch(ti_thing_t * thing, ti_stream_t * stream);
int ti_thing__to_pk(ti_thing_t * thing, msgpack_packer * pk, int options);
int ti_thing_t_to_pk(ti_thing_t * thing, msgpack_packer * pk, int options);
_Bool ti__thing_has_watchers_(ti_thing_t * thing);
int ti_thing_o_set_val_from_strn(
        ti_wprop_t * wprop,
        ti_thing_t * thing,
        const char * str,
        size_t n,
        ti_val_t ** val,
        ex_t * e);
int ti_thing_t_set_val_from_strn(
        ti_wprop_t * wprop,
        ti_thing_t * thing,
        const char * str,
        size_t n,
        ti_val_t ** val,
        ex_t * e);

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
    vec_t * items;                  /* vec contains ti_prop_t */
    vec_t * watchers;               /* vec contains ti_watch_t,
                                       NULL if no watchers,  */
};


static inline _Bool ti_thing_is_object(ti_thing_t * thing)
{
    return thing->type_id == TI_SPEC_OBJECT;
}

static inline _Bool ti_thing_is_instance(ti_thing_t * thing)
{
    return thing->type_id != TI_SPEC_OBJECT;
}

static inline _Bool ti_thing_has_watchers(ti_thing_t * thing)
{
    return thing->watchers && ti__thing_has_watchers_(thing);
}

static inline int ti_thing_id_to_pk(ti_thing_t * thing, msgpack_packer * pk)
{
    return -(msgpack_pack_map(pk, !!thing->id) ||
        (thing->id && (
            msp_pack_strn(pk, TI_KIND_S_THING, 1) ||
            msgpack_pack_uint64(pk, thing->id)
        ))
    );
}

static inline _Bool ti_thing_is_new(ti_thing_t * thing)
{
    return thing->flags & TI_VFLAG_THING_NEW;
}
static inline void ti_thing_mark_new(ti_thing_t * thing)
{
    thing->flags |= TI_VFLAG_THING_NEW;
}
static inline void ti_thing_unmark_new(ti_thing_t * thing)
{
    thing->flags &= ~TI_VFLAG_THING_NEW;
}
static inline uint64_t ti_thing_key(ti_thing_t * thing)
{
    return (uintptr_t) thing;
}

static inline ti_prop_t * ti_thing_o_prop_weak_get(
        ti_thing_t * thing,
        ti_name_t * name)
{
    assert (ti_thing_is_object(thing));
    for (vec_each(thing->items, ti_prop_t, prop))
        if (prop->name == name)
            return prop;
    return NULL;
}

#define thing_each(t__, name__, val__)                          \
    void ** v__ = t__->items->data,                             \
    ** e__ = v__ + t__->items->n,                               \
    ** n__ = ti_thing_type(t__)->fields->data;                  \
    v__ < e__ &&                                                \
    (name__ = ((ti_field_t *) *n__)->name) &&                   \
    (val__ = *v__);                                             \
    ++v__, ++n__

#define thing_each_addr(t__, name__, val__)                     \
    void ** v__ = t__->items->data,                             \
    ** e__ = v__ + t__->items->n,                               \
    ** n__ = ti_thing_type(t__)->fields->data;                  \
    v__ < e__ &&                                                \
    (name__ = ((ti_field_t *) *n__)->name) &&                   \
    (val__ = (ti_val_t **) v__);                                \
    ++v__, ++n__

#endif /* TI_THING_H_ */
