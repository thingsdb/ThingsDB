/*
 * ti/things.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/collection.inline.h>
#include <ti/enum.h>
#include <ti/field.h>
#include <ti/names.h>
#include <ti/prop.h>
#include <ti/thing.inline.h>
#include <ti/things.h>
#include <ti/types.inline.h>
#include <ti/val.h>
#include <ti/val.inline.h>
#include <ti/varr.h>
#include <ti/vset.h>
#include <ti/watch.h>
#include <util/logger.h>

ti_thing_t * ti_things_create_thing_o(
        uint64_t id,
        size_t init_sz,
        ti_collection_t * collection)
{
    assert (id);
    ti_thing_t * thing = ti_thing_o_create(id, init_sz, collection);
    if (!thing || ti_thing_to_map(thing))
    {
        ti_val_drop((ti_val_t *) thing);
        return NULL;
    }
    return thing;
}

ti_thing_t * ti_things_create_thing_t(
        uint64_t id,
        ti_type_t * type,
        ti_collection_t * collection)
{
    assert (id);
    ti_thing_t * thing = ti_thing_t_create(id, type, collection);
    if (!thing || ti_thing_to_map(thing))
    {
        ti_val_drop((ti_val_t *) thing);
        return NULL;
    }
    return thing;
}

/* Returns a thing with a new reference or NULL in case of an error */
ti_thing_t * ti_things_thing_o_from_vup(
        ti_vup_t * vup,
        uint64_t thing_id,
        size_t sz,
        ex_t * e)
{
    ti_thing_t * thing;
    thing = ti_collection_find_thing(vup->collection, thing_id);
    if (thing)
    {
        if (sz != 1)
        {
            ex_set(e, EX_BAD_DATA,
                    "cannot directly assign properties to "TI_THING_ID" "
                    "by adding properties to the data; "
                    "change the thing to {\""TI_KIND_S_THING"\": %"PRIu64"}",
                    thing_id, thing_id);
            return NULL;
        }
        ti_incref(thing);
        return thing;
    }

    --sz;  /* decrease one to unpack the remaining properties */

    thing = ti_things_create_thing_o(thing_id, sz, vup->collection);
    if (!thing)
    {
        ex_set_mem(e);
        return NULL;
    }

    ti_update_next_thing_id(thing_id);

    if (ti_thing_props_from_vup(thing, vup, sz, e))
    {
        ti_val_unsafe_drop((ti_val_t *) thing);
        return NULL;
    }
    return thing;
}

/* Returns a thing with a new reference or NULL in case of an error */
ti_thing_t * ti_things_thing_t_from_vup(ti_vup_t * vup, ex_t * e)
{
    ti_thing_t * thing;
    ti_type_t * type;
    mp_obj_t obj, mp_thing_id, mp_type_id;

    if (mp_next(vup->up, &mp_type_id) != MP_U64 ||
        mp_skip(vup->up) != MP_STR ||   /* `#` */
        mp_next(vup->up, &mp_thing_id) != MP_U64 ||
        mp_skip(vup->up) != MP_STR ||   /* `` */
        mp_next(vup->up, &obj) != MP_ARR)
    {
        ex_set(e, EX_BAD_DATA,
                "invalid type data; "
                "expecting an type_id, things_id and array with values");
        return NULL;
    }

    type = ti_types_by_id(vup->collection->types, mp_type_id.via.u64);
    if (!type)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "invalid type data; type id %"PRIu64" not found",
                mp_type_id.via.u64);
        return NULL;
    }

    if (obj.via.sz != type->fields->n)
    {
        ex_set(e, EX_BAD_DATA,
                "invalid type data; "
                "expecting %"PRIu32" values for type `%s` but got only %u",
                type->fields->n, type->name, obj.via.sz);
        return NULL;
    }

    thing = ti_things_create_thing_t(
            mp_thing_id.via.u64,
            type,
            vup->collection);

    if (!thing)
    {
        /* No need to check for garbage collected things */
        if (ti_collection_thing_by_id(vup->collection, mp_thing_id.via.u64))
            ex_set(e, EX_LOOKUP_ERROR,
                    "error while loading type `%s`; "
                    "thing "TI_THING_ID" already exists",
                    type->name, mp_thing_id.via.u64);
        else
            ex_set_mem(e);
        return NULL;
    }

    /* Update the next thing id if required */
    ti_update_next_thing_id(mp_thing_id.via.u64);

    for (vec_each(type->fields, ti_field_t, field))
    {
        ti_val_t * val = ti_val_from_vup_e(vup, e);

        if (!val || ti_field_make_assignable(field, &val, thing, e))
        {
            ex_append(e, "; error while loading field `%s` for type `%s`",
                    field->name->str,
                    type->name);
            ti_val_drop(val);
            ti_val_unsafe_drop((ti_val_t *) thing);
            return NULL;
        }

        VEC_push(thing->items.vec, val);
    }

    return thing;
}
