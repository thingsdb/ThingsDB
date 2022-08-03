/*
 * ti/thing.inline.h
 */
#ifndef TI_THING_INLINE_H_
#define TI_THING_INLINE_H_

#include <ti/type.h>
#include <ti/name.h>
#include <ti/names.h>
#include <ti/raw.h>
#include <ti/raw.inline.h>
#include <ti/thing.h>
#include <ti/thing.t.h>
#include <ti/vp.t.h>
#include <ti/witem.t.h>
#include <util/strx.h>
#include <doc.h>

/* returns IMAP_ERR_EXIST if the thing is already in the map */
static inline int ti_thing_to_map(ti_thing_t * thing)
{
    return imap_add(thing->collection->things, thing->id, thing);
}

static inline const char * ti_thing_type_str(ti_thing_t * thing)
{
    return thing->via.type->name;
}

static inline ti_raw_t * ti_thing_type_strv(ti_thing_t * thing)
{
    ti_raw_t * r = thing->via.type->rname;
    ti_incref(r);
    return r;
}

static inline int ti_thing_set_val_from_strn(
        ti_witem_t * witem,
        ti_thing_t * thing,
        const char * str,
        size_t n,
        ti_val_t ** val,
        ex_t * e)
{
    return ti_thing_is_object(thing)
            ? ti_thing_o_set_val_from_strn(witem, thing, str, n, val, e)
            : ti_thing_t_set_val_from_strn(
                    (ti_wprop_t *) witem,
                    thing, str, n, val, e);
}

static inline int ti_thing_set_val_from_valid_strn(
        ti_wprop_t * wprop,
        ti_thing_t * thing,
        const char * str,
        size_t n,
        ti_val_t ** val,
        ex_t * e)
{
    return ti_thing_is_object(thing)
            ? ti_thing_o_set_val_from_valid_strn(wprop, thing, str, n, val, e)
            : ti_thing_t_set_val_from_strn(wprop, thing, str, n, val, e);
}


static inline void ti_thing_o_set_not_found(
        ti_thing_t * thing,
        ti_raw_t * key,
        ex_t * e)
{
    if (!strx_is_utf8n((const char *) key->data, key->n))
    {
        ex_set(e, EX_VALUE_ERROR, "properties must have valid UTF-8 encoding");
    }
    else
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "thing "TI_THING_ID" has no property `%s`",
                thing->id,
                ti_raw_as_printable_str(key));
    }
}

static inline void ti_thing_t_set_not_found(
        ti_thing_t * thing,
        ti_name_t * name,
        ti_raw_t * rname,
        ex_t * e)
{
    if (name || ti_name_is_valid_strn((const char *) rname->data, rname->n))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no property or method `%.*s`",
                thing->via.type->name,
                rname->n, (const char *) rname->data);
    }
    else
    {
        ex_set(e, EX_VALUE_ERROR,
                "type keys must follow the naming rules"DOC_NAMES);
    }
}

static inline ti_val_t * ti_thing_p_val_weak_get(
        ti_thing_t * thing,
        ti_name_t * name)
{
    for (vec_each(thing->items.vec, ti_prop_t, prop))
        if (prop->name == name)
            return prop->val;
    return NULL;
}

static inline ti_val_t * ti_thing_o_val_weak_get(
        ti_thing_t * thing,
        ti_name_t * name)
{
    if (ti_thing_is_dict(thing))
    {
        ti_item_t * item = smap_get(thing->items.smap, name->str);
        return item ? item->val : NULL;
    }
    return ti_thing_p_val_weak_get(thing, name);
}

static inline ti_val_t * ti_thing_t_val_weak_get(
        ti_thing_t * thing,
        ti_name_t * name)
{
    ti_name_t * n;
    ti_val_t * v;
    for (thing_t_each(thing, n, v))
        if (n == name)
            return v;
    return NULL;
}

static inline ti_val_t * ti_thing_val_weak_by_name(
        ti_thing_t * thing,
        ti_name_t * name)
{
    return ti_thing_is_object(thing)
            ? ti_thing_o_val_weak_get(thing, name)
            : ti_thing_t_val_weak_get(thing, name);
}

static inline int ti_thing_to_client_pk(
        ti_thing_t * thing,
        ti_vp_t * vp,
        int deep,
        int flags)
{
    return (!deep || (thing->flags & TI_VFLAG_LOCK))
            ? (!thing->id || (flags & TI_FLAGS_NO_IDS))
            ? ti_thing_empty_to_client_pk(thing, &vp->pk)
            : ti_thing_id_to_client_pk(thing, &vp->pk)
            : ti_thing__to_client_pk(thing, vp, deep, flags);
}

static inline int ti_thing_to_store_pk(ti_thing_t * thing, msgpack_packer * pk)
{
    if (!ti_thing_is_new(thing))
    {
        unsigned char buf[8];
        mp_store_uint64(thing->id, buf);
        return mp_pack_ext(pk, MPACK_EXT_THING, buf, sizeof(buf));
    }

    ti_thing_unmark_new(thing);

    return ti_thing_is_object(thing)
            ? ti_thing_o_to_pk(thing, pk)
            : ti_thing_t_to_pk(thing, pk);
}

static inline void ti_thing_may_push_gc(ti_thing_t * thing)
{
    if (    thing->tp == TI_VAL_THING &&
            thing->id == 0 &&
            (thing->flags & TI_THING_FLAG_SWEEP) &&
            vec_push(&ti_thing_gc_vec, thing) == 0)
    {
        ti_incref(thing);
        thing->flags &= ~TI_THING_FLAG_SWEEP;
    }
}

/*
 * Does not increment the `name` and `val` reference counters.
 */
static inline int ti_thing_o_set(
        ti_thing_t * thing,
        ti_raw_t * key,
        ti_val_t * val)
{
    if (ti_thing_is_dict(thing))
        return -!ti_thing_i_item_set(thing, key, val);

    if (ti_raw_is_name(key))
        return -!ti_thing_p_prop_set(thing, (ti_name_t *) key, val);

    return -(ti_thing_to_dict(thing) || !ti_thing_i_item_set(thing, key, val));
}

/*
 * Does not increment the `name` and `val` reference counters.
 * Use only when you are sure the property does not yet exist.
 */
static inline int ti_thing_o_add(
        ti_thing_t * thing,
        ti_raw_t * key,
        ti_val_t * val)
{
    if (ti_thing_is_dict(thing))
        return -!ti_thing_i_item_add(thing, key, val);

    if (ti_raw_is_name(key))
        return -!ti_thing_p_prop_add(thing, (ti_name_t *) key, val);

    return -(ti_thing_to_dict(thing) || !ti_thing_i_item_add(thing, key, val));
}

static inline _Bool ti_thing_has_key(ti_thing_t * thing, ti_raw_t * key)
{
    if (!ti_thing_is_dict(thing))
    {
        ti_name_t * name = ti_names_weak_from_raw(key);
        return name && ti_thing_val_weak_by_name(thing, name);
    }
    return !!smap_getn(thing->items.smap, (const char *) key->data, key->n);
}

static inline ti_raw_t * ti_thing_str(ti_thing_t * thing)
{
    return ti_thing_is_object(thing)
        ? thing->id
        ? ti_str_from_fmt("thing:%"PRIu64, thing->id)
        : ti_str_from_str("thing:nil")
        : thing->id
        ? ti_str_from_fmt("%s:%"PRIu64, thing->via.type->name, thing->id)
        : ti_str_from_fmt("%s:nil", thing->via.type->name);
}

#endif  /* TI_THING_INLINE_H_ */
