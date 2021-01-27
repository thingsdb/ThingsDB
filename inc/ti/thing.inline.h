/*
 * ti/thing.inline.h
 */
#ifndef TI_THING_INLINE_H_
#define TI_THING_INLINE_H_

#include <ti/type.h>
#include <ti/thing.h>
#include <ti/thing.t.h>
#include <ti/witem.t.h>
#include <ti/name.h>
#include <ti/raw.h>
#include <util/strx.h>
#include <doc.h>

/* returns IMAP_ERR_EXIST if the thing is already in the map */
static inline int ti_thing_to_map(ti_thing_t * thing)
{
    return imap_add(thing->collection->things, thing->id, thing);
}

static inline ti_type_t * ti_thing_type(ti_thing_t * thing)
{
    ti_type_t * type = imap_get(thing->collection->types->imap, thing->type_id);
    assert (type);  /* type are guaranteed to exist */
    return type;
}

static inline const char * ti_thing_type_str(ti_thing_t * thing)
{
    return ti_thing_type(thing)->name;
}

static inline ti_raw_t * ti_thing_type_strv(ti_thing_t * thing)
{
    ti_raw_t * r = ti_thing_type(thing)->rname;
    ti_incref(r);
    return r;
}
static inline int ti_thing_o_set_val_from_strn(
        ti_witem_t * witem,
        ti_thing_t * thing,
        const char * str,
        size_t n,
        ti_val_t ** val,
        ex_t * e)
{
    if (ti_name_is_valid_strn(str, n))
        return ti_thing_o_set_val_from_valid_strn(
                (ti_wprop_t *) witem,
                thing, str, n, val, e);

    /*
     *  TODO: UTF-8 encoding depends on correct msgpack data, should we add an
     *  extra check?
     */

    if (!strx_is_utf8n(str, n))
    {
        ex_set(e, EX_VALUE_ERROR, "properties must have valid UTF-8 encoding");
        return e->nr;
    }

    if (!ti_thing_is_object_i(thing) && ti_thing_to_items(thing))
    {
        ex_set_mem(e);
        return e->nr;
    }

    return ti_thing_i_set_val_from_strn(witem, thing, str, n, val, e);
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
        size_t n = key->n <= 30 ? key->n : 30;
        ex_set(e, EX_LOOKUP_ERROR,
                "thing "TI_THING_ID" has no property `%.*s%s`",
                thing->id,
                n,
                (const char *) key->data,
                key->n == n ? "" : "...");
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
                ti_thing_type(thing)->name,
                (int) rname->n, (const char *) rname->data);
    }
    else
    {
        ex_set(e, EX_VALUE_ERROR,
                "type properties name must follow the naming rules"DOC_NAMES);
    }
}

static inline ti_val_t * ti_thing_o_val_weak_get(
        ti_thing_t * thing,
        ti_name_t * name)
{
    assert (ti_thing_is_object(thing));
    for (vec_each(thing->items.vec, ti_prop_t, prop))
        if (prop->name == name)
            return prop->val;
    return NULL;
}

static inline ti_val_t * ti_thing_t_val_weak_get(
        ti_thing_t * thing,
        ti_name_t * name)
{
    assert (ti_thing_is_instance(thing));
    ti_name_t * n;
    ti_val_t * v;
    for (thing_t_each(thing, n, v))
        if (n == name)
            return v;
    return NULL;
}

static inline ti_val_t * ti_thing_val_weak_get(ti_thing_t * thing, ti_name_t * name)
{
    return ti_thing_is_object(thing)
            ? ti_thing_o_val_weak_get(thing, name)
            : ti_thing_t_val_weak_get(thing, name);
}

static inline int ti_thing_to_pk(
        ti_thing_t * thing,
        msgpack_packer * pk,
        int options)
{
    if (options == TI_VAL_PACK_TASK)
    {
        if (ti_thing_is_new(thing))
        {
            ti_thing_unmark_new(thing);
            return ti_thing_is_object(thing)
                    ? ti_thing__to_pk(thing, pk, options)
                    : ti_thing_t_to_pk(thing, pk, options);
        }
    }
    return options <= 0 || (thing->flags & TI_VFLAG_LOCK)
            ? ti_thing_id_to_pk(thing, pk)
            : ti_thing__to_pk(thing, pk, options);
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

#endif  /* TI_THING_INLINE_H_ */
