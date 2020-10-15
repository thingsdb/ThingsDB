/*
 * ti/wrap.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/closure.h>
#include <ti/field.h>
#include <ti/mapping.h>
#include <ti/member.h>
#include <ti/regex.h>
#include <ti/types.inline.h>
#include <ti/val.inline.h>
#include <ti/vbool.h>
#include <ti/verror.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <ti/vset.h>
#include <ti/wrap.h>
#include <ti/wrap.inline.h>
#include <util/vec.h>

ti_wrap_t * ti_wrap_create(ti_thing_t * thing, uint16_t type_id)
{
    ti_wrap_t * wrap = malloc(sizeof(ti_wrap_t));
    if (!wrap)
        return NULL;

    wrap->ref = 1;
    wrap->tp = TI_VAL_WRAP;
    wrap->type_id = type_id;
    wrap->thing = thing;

    ti_incref(thing);

    return wrap;
}

void ti_wrap_destroy(ti_wrap_t * wrap)
{
    if (!wrap)
        return;
    ti_val_unsafe_gc_drop((ti_val_t *) wrap->thing);
    free(wrap);
}

typedef struct
{
    msgpack_packer * pk;
    uint16_t spec;
    uint16_t _pad0;
    int options;
} wrap__walk_t;

static int wrap__walk(ti_thing_t * thing, wrap__walk_t * w)
{
    return ti__wrap_field_thing(thing, w->pk, w->spec, w->options);
}

static int wrap__set(
        ti_vset_t * vset,
        msgpack_packer * pk,
        uint16_t spec,
        int options)
{
    wrap__walk_t w = {
            .pk = pk,
            .spec = spec,
            .options = options,
    };

    return (
            msgpack_pack_array(pk, vset->imap->n) ||
            imap_walk(vset->imap, (imap_cb) wrap__walk, &w)
    );
}

static int wrap__field_val(
        ti_field_t * t_field,
        uint16_t * spec,    /* points to t_field->spec or t_field->nested */
        ti_val_t * val,
        msgpack_packer * pk,
        int options)
{
    switch ((ti_val_enum) val->tp)
    {
    TI_VAL_PACK_CASE_IMMUTABLE(val, pk, options)
    case TI_VAL_THING:
        return ti__wrap_field_thing(
                (ti_thing_t *) val,
                pk,
                *spec,
                options);
    case TI_VAL_WRAP:
        return ti__wrap_field_thing(
                ((ti_wrap_t *) val)->thing,
                pk,
                *spec,
                options);
    case TI_VAL_ARR:
    {
        ti_varr_t * varr = (ti_varr_t *) val;
        if (msgpack_pack_array(pk, varr->vec->n))
            return -1;
        for (vec_each(varr->vec, ti_val_t, v))
        {
            if (wrap__field_val(
                    t_field,
                    &t_field->nested_spec,
                    v,
                    pk,
                    options))
                return -1;
        }
        return 0;
    }
    case TI_VAL_SET:
        return wrap__set(
                (ti_vset_t *) val,
                pk,
                t_field->nested_spec,
                options);
    case TI_VAL_MEMBER:
        return wrap__field_val(
                t_field,
                spec,
                VMEMBER(val),
                pk,
                options);
    }

    assert(0);
    return -1;
}

static inline int wrap__thing_id_to_pk(
        ti_thing_t * thing,
        msgpack_packer * pk,
        size_t n)
{
    if (msgpack_pack_map(pk, (!!thing->id) + n))
        return -1;

    if (thing->id && (
            mp_pack_strn(pk, TI_KIND_S_THING, 1) ||
            msgpack_pack_uint64(pk, thing->id)
    )) return -1;

    return 0;
}

/*
 * Do not use directly, use ti_wrap_to_pk() instead
 */
int ti__wrap_field_thing(
        ti_thing_t * thing,
        msgpack_packer * pk,
        uint16_t spec,
        int options)
{
    ti_type_t * t_type;
    spec &= TI_SPEC_MASK_NILLABLE;

    assert (options >= 0);
    assert (thing->tp == TI_VAL_THING);
    assert (spec <= TI_SPEC_OBJECT);

    /*
     * Just return the ID when locked or if `options` (deep) has reached zero.
     */
    if ((thing->flags & TI_VFLAG_LOCK) || !options)
        return ti_thing_id_to_pk(thing, pk);

    /*
     * If `spec` is not a type or a none existing type (thus ANY or OBJECT),
     * then pack the thing as normal.
     */
    if (spec >= TI_SPEC_ANY ||  /* TI_SPEC_ANY || TI_SPEC_OBJECT */
        !(t_type = ti_types_by_id(thing->collection->types, spec)))
        return ti_thing__to_pk(thing, pk, options);

    /* decrement `options` (deep) by one */
    --options;

    /* Set the lock */
    thing->flags |= TI_VFLAG_LOCK;

    if (ti_thing_is_object(thing))
    {
        /* If the `thing` to wrap is not an existing type but a normal `thing`,
         * some extra work needs to be done.
         */
        typedef struct
        {
            ti_field_t * field;
            ti_prop_t * prop;
        } map_prop_t;
        size_t n = ti_min(t_type->fields->n, thing->items->n);
        map_prop_t * map_props = malloc(sizeof(map_prop_t) * n);
        map_prop_t * map_set = map_props;
        map_prop_t * map_get = map_props;
        if (!map_props)
            goto fail;

        n = 0;
        /*
         * First read all the appropriate properties to allocated `map_prop_t`.
         * There is enough room allocated as it will never exceed more than
         * there are properties on the source or fields in the target.
         */
        for (vec_each(t_type->fields, ti_field_t, t_field))
        {
            ti_prop_t * prop;
            prop = ti_thing_o_prop_weak_get(thing, t_field->name);
            if (!prop || !ti_field_maps_to_val(t_field, prop->val))
                continue;

            map_set->field = t_field;
            map_set->prop = prop;
            ++map_set;
            ++n;
        };

        /*
         * Now we can pack, let's start with the ID.
         */
        if (wrap__thing_id_to_pk(thing, pk, n))
        {
            free(map_props);
            goto fail;
        }

        /*
         * Pack all the gathered properties.
         */
        for (;map_get < map_set; ++map_get)
        {
            if (mp_pack_strn(
                    pk,
                    map_get->prop->name->str,
                    map_get->prop->name->n) ||
                wrap__field_val(
                        map_get->field,
                        &map_get->field->spec,
                        map_get->prop->val,
                        pk,
                        options)
            ) {
                free(map_props);
                goto fail;
            }
        }

        free(map_props);
    }
    else
    {
        vec_t * mappings;
        ti_type_t * f_type = ti_thing_type(thing);

        /*
         * Type mappings are only created the first time a conversion from
         * `to_type` -> `from_type` is asked so most likely the mappings are
         * returned from cache.
         */
        mappings = ti_type_map(t_type, f_type);
        if (!mappings || wrap__thing_id_to_pk(thing, pk, mappings->n))
            goto fail;

        for (vec_each(mappings, ti_mapping_t, mapping))
        {
            if (mp_pack_strn(
                        pk,
                        mapping->f_field->name->str,
                        mapping->f_field->name->n) ||
                wrap__field_val(
                        mapping->t_field,
                        &mapping->t_field->spec,
                        VEC_get(thing->items, mapping->f_field->idx),
                        pk,
                        options)
            ) goto fail;
        }
    }

    thing->flags &= ~TI_VFLAG_LOCK;
    return 0;
fail:
    thing->flags &= ~TI_VFLAG_LOCK;
    return -1;
}


