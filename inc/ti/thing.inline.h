/*
 * ti/thing.inline.h
 */
#ifndef TI_THING_INLINE_H_
#define TI_THING_INLINE_H_

#include <ti/type.h>
#include <ti/thing.h>
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

static inline int ti_thing_set_val_from_strn(
        ti_wprop_t * wprop,
        ti_thing_t * thing,
        const char * str,
        size_t n,
        ti_val_t ** val,
        ex_t * e)
{
    return ti_thing_is_object(thing)
            ? ti_thing_o_set_val_from_strn(wprop, thing, str, n, val, e)
            : ti_thing_t_set_val_from_strn(wprop, thing, str, n, val, e);
}

static inline void ti_thing_set_not_found(
        ti_thing_t * thing,
        ti_name_t * name,
        ti_raw_t * rname,
        ex_t * e)
{
    if (name || ti_name_is_valid_strn((const char *) rname->data, rname->n))
    {
        if (ti_thing_is_object(thing))
            ex_set(e, EX_LOOKUP_ERROR,
                    "thing "TI_THING_ID" has no property `%.*s`",
                    thing->id,
                    (int) rname->n, (const char *) rname->data);
        else
            ex_set(e, EX_LOOKUP_ERROR,
                    "type `%s` has no property `%.*s`",
                    ti_thing_type(thing)->name,
                    (int) rname->n, (const char *) rname->data);
    }
    else
    {
        ex_set(e, EX_VALUE_ERROR,
                "property name must follow the naming rules"DOC_NAMES);
    }
}

static inline ti_val_t * ti_thing_o_val_weak_get(
        ti_thing_t * thing,
        ti_name_t * name)
{
    assert (ti_thing_is_object(thing));
    for (vec_each(thing->items, ti_prop_t, prop))
        if (prop->name == name)
            return prop->val;
    return NULL;
}

static inline ti_val_t * ti_thing_t_val_weak_get(
        ti_thing_t * thing,
        ti_name_t * name)
{
    assert (!ti_thing_is_object(thing));
    ti_name_t * n;
    ti_val_t * v;
    for (thing_each(thing, n, v))
        if (n == name)
            return v;
    return NULL;
}

static inline ti_val_t * ti_thing_val_weak_get(
        ti_thing_t * thing,
        ti_name_t * name)
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

#endif  /* TI_THING_INLINE_H_ */
