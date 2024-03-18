/*
 * ti/thing.h
 */
#ifndef TI_THING_H_
#define TI_THING_H_

#include <assert.h>
#include <stdint.h>
#include <ti/collection.t.h>
#include <ti/field.t.h>
#include <ti/item.t.h>
#include <ti/name.t.h>
#include <ti/prop.t.h>
#include <ti/raw.t.h>
#include <ti/spec.t.h>
#include <ti/stream.t.h>
#include <ti/task.t.h>
#include <ti/thing.t.h>
#include <ti/type.t.h>
#include <ti/val.t.h>
#include <ti/vp.t.h>
#include <ti/vup.t.h>
#include <ti/watch.t.h>
#include <ti/witem.t.h>
#include <ti/wprop.t.h>
#include <util/mpack.h>
#include <util/vec.h>

typedef int (*ti_thing_item_cb) (ti_raw_t *, ti_val_t *, void *);

ti_thing_t * ti_thing_o_create(
        uint64_t id,
        size_t init_sz,
        ti_collection_t * collection);
ti_thing_t * ti_thing_i_create(uint64_t id, ti_collection_t * collection);
ti_thing_t * ti_thing_t_create(
        uint64_t id,
        ti_type_t * type,
        ti_collection_t * collection);
void ti_thing_destroy(ti_thing_t * thing);
void ti_thing_clear(ti_thing_t * thing);
void ti_thing_o_items_destroy(ti_thing_t * thing);
int ti_thing_to_dict(ti_thing_t * thing);
int ti_thing_i_to_p(ti_thing_t * thing, ti_raw_t ** incompatible);
int ti_thing_props_from_vup(
        ti_thing_t * thing,
        ti_vup_t * vup,
        size_t sz,
        ex_t * e);
ti_thing_t * ti_thing_new_from_vup(ti_vup_t * vup, size_t sz, ex_t * e);
ti_prop_t * ti_thing_p_prop_add(    /* only when property does not exists */
        ti_thing_t * thing,
        ti_name_t * name,
        ti_val_t * val);
int ti_thing_p_prop_add_assign(
        ti_thing_t * thing,
        ti_name_t * name,
        ti_val_t * val,
        ex_t * e);
ti_item_t * ti_thing_i_item_add(
        ti_thing_t * thing,
        ti_raw_t * key,
        ti_val_t * val);
int ti_thing_i_item_add_assign(
        ti_thing_t * thing,
        ti_raw_t * key,
        ti_val_t * val,
        ex_t * e);
ti_prop_t * ti_thing_p_prop_set(
        ti_thing_t * thing,
        ti_name_t * name,
        ti_val_t * val);
ti_item_t * ti_thing_i_item_set(
        ti_thing_t * thing,
        ti_raw_t * key,
        ti_val_t * val);
void ti_thing_t_prop_set(
        ti_thing_t * thing,
        ti_field_t * field,
        ti_val_t * val);
void ti_thing_t_to_object(ti_thing_t * thing);
void ti_thing_o_del(ti_thing_t * thing, const char * str, size_t n);
ti_item_t * ti_thing_o_del_e(ti_thing_t * thing, ti_raw_t * rname, ex_t * e);
_Bool ti_thing_get_by_raw(ti_witem_t * witem, ti_thing_t * thing, ti_raw_t * raw);
int ti_thing_get_by_raw_e(
        ti_witem_t * witem,
        ti_thing_t * thing,
        ti_raw_t * r,
        ex_t * e);
int ti_thing_gen_id(ti_thing_t * thing);
_Bool ti_thing_has_id(ti_thing_t * thing);
int ti_thing__to_client_pk(
        ti_thing_t * thing,
        ti_vp_t * vp,
        int deep,
        int flags);
int ti_thing_o_to_pk(ti_thing_t * thing, msgpack_packer * pk);
int ti_thing_t_to_pk(ti_thing_t * thing, msgpack_packer * pk);
ti_val_t * ti_thing_val_by_strn(ti_thing_t * thing, const char * str, size_t n);
_Bool ti_thing_equals(ti_thing_t * thing, ti_val_t * other, uint8_t deep);
int ti_thing_i_set_val_from_strn(
        ti_witem_t * witem,
        ti_thing_t * thing,
        const char * str,
        size_t n,
        ti_val_t ** val,
        ex_t * e);
int ti_thing_o_set_val_from_strn(
        ti_witem_t * witem,
        ti_thing_t * thing,
        const char * str,
        size_t n,
        ti_val_t ** val,
        ex_t * e);
int ti_thing_o_set_val_from_valid_strn(
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
int ti_thing_init_gc(void);
void ti_thing_destroy_gc(void);
void ti_thing_clean_gc(void);
void ti_thing_resize_gc(void);
int ti_thing_copy(ti_thing_t ** thing, uint8_t deep);
int ti_thing_dup(ti_thing_t ** thing, uint8_t deep);
void ti_thing_cancel(ti_thing_t * thing);
int ti_thing_assign(
        ti_thing_t * thing,
        ti_thing_t * tsrc,
        ti_task_t * task,
        ex_t * e);
int ti_thing_walk(ti_thing_t * thing, ti_thing_item_cb cb, void * data);
int ti_thing_o_ren(
        ti_thing_t * thing,
        const char * ostr,
        size_t on,
        const char * nstr,
        size_t nn,
        ex_t * e);

#if TI_IS64BIT
#define THING__KEY_SHIFT 3
#else
#define THING__KEY_SHIFT 2
#endif

static inline _Bool ti_thing_is_object(ti_thing_t * thing)
{
    return thing->type_id == TI_SPEC_OBJECT;
}

static inline _Bool ti_thing_o_is_restricted(ti_thing_t * thing)
{
    return thing->via.spec != TI_SPEC_ANY;
}

static inline _Bool ti_thing_is_dict(ti_thing_t * thing)
{
    return thing->flags & TI_THING_FLAG_DICT;
}

static inline _Bool ti_thing_is_instance(ti_thing_t * thing)
{
    return thing->type_id != TI_SPEC_OBJECT;
}

static inline int ti_thing_id_to_client_pk(ti_thing_t * thing, msgpack_packer * pk)
{
    register const ti_name_t * name = ti_thing_is_instance(thing)
            ? thing->via.type->idname
            : NULL;
    return -(
            msgpack_pack_map(pk,1) || (name
                ? mp_pack_strn(pk, name->str, name->n)
                : mp_pack_strn(pk, TI_KIND_S_THING, 1)) ||
            msgpack_pack_uint64(pk, thing->id)
    );
}

static inline int ti_thing_empty_to_client_pk(msgpack_packer * pk)
{
    return msgpack_pack_map(pk, 0);
}

static inline _Bool ti_thing_is_new(ti_thing_t * thing)
{
    return thing->flags & TI_THING_FLAG_NEW;
}
static inline void ti_thing_mark_new(ti_thing_t * thing)
{
    thing->flags |= TI_THING_FLAG_NEW;
}
static inline void ti_thing_unmark_new(ti_thing_t * thing)
{
    thing->flags &= ~TI_THING_FLAG_NEW;
}
static inline uint64_t ti_thing_key(ti_thing_t * thing)
{
    return (uintptr_t) thing >> THING__KEY_SHIFT;
}
static inline uint32_t ti_thing_n(ti_thing_t * thing)
{
    /* both smap_t and vec_t have the `n` counter on top */
    return thing->items.vec->n;
}

static inline ti_prop_t * ti_thing_o_prop_weak_get(
        ti_thing_t * thing,
        ti_name_t * name)
{
    if (ti_thing_is_dict(thing))
        return smap_get(thing->items.smap, name->str);

    for (vec_each(thing->items.vec, ti_prop_t, prop))
        if (prop->name == name)
            return prop;
    return NULL;
}

#define thing_t_each(t__, name__, val__)                        \
    void ** v__ = t__->items.vec->data,                         \
    ** e__ = v__ + t__->items.vec->n,                           \
    ** n__ = (t__)->via.type->fields->data;                     \
    v__ < e__ &&                                                \
    (name__ = ((ti_field_t *) *n__)->name) &&                   \
    (val__ = *v__);                                             \
    ++v__, ++n__

#define thing_t_each_addr(t__, name__, val__)                   \
    void ** v__ = t__->items.vec->data,                         \
    ** e__ = v__ + t__->items.vec->n,                           \
    ** n__ = (t__)->via.type->fields->data;                     \
    v__ < e__ &&                                                \
    (name__ = ((ti_field_t *) *n__)->name) &&                   \
    (val__ = (ti_val_t **) v__);                                \
    ++v__, ++n__

#endif /* TI_THING_H_ */
