/*
 * ti/wrap.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/closure.h>
#include <ti/field.h>
#include <ti/future.h>
#include <ti/mapping.h>
#include <ti/member.h>
#include <ti/method.t.h>
#include <ti/prop.h>
#include <ti/regex.h>
#include <ti/room.inline.h>
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
    ti_vp_t * vp;
    uint16_t spec;
    uint16_t _pad0;
    int options;
} wrap__walk_t;

static int wrap__walk(ti_thing_t * thing, wrap__walk_t * w)
{
    return ti__wrap_field_thing(thing, w->vp, w->spec, w->options);
}

static int wrap__set(
        ti_vset_t * vset,
        ti_vp_t * vp,
        uint16_t spec,
        int options)
{
    wrap__walk_t w = {
            .vp = vp,
            .spec = spec,
            .options = options,
    };

    return (
            msgpack_pack_array(&vp->pk, vset->imap->n) ||
            imap_walk(vset->imap, (imap_cb) wrap__walk, &w)
    );
}

static int wrap__field_val(
        ti_field_t * t_field,
        uint16_t * spec,    /* points to t_field->spec or t_field->nested */
        ti_val_t * val,
        ti_vp_t * vp,
        int options)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:
        return msgpack_pack_nil(&vp->pk);
    case TI_VAL_INT:
        return msgpack_pack_int64(&vp->pk, VINT(val));
    case TI_VAL_FLOAT:
        return msgpack_pack_double(&vp->pk, VFLOAT(val));
    case TI_VAL_BOOL:
        return ti_vbool_to_pk((ti_vbool_t *) val, &vp->pk);
    case TI_VAL_DATETIME:
        return ti_datetime_to_pk((ti_datetime_t *) val, &vp->pk, options);
    case TI_VAL_NAME:
    case TI_VAL_STR:
        return ti_raw_str_to_pk((ti_raw_t *) val, &vp->pk);
    case TI_VAL_BYTES:
        return ti_raw_bytes_to_pk((ti_raw_t *) val, &vp->pk);
    case TI_VAL_REGEX:
        return ti_regex_to_pk((ti_regex_t *) val, &vp->pk, options);
    case TI_VAL_THING:
        return ti__wrap_field_thing(
                (ti_thing_t *) val,
                vp,
                *spec,
                options);
    case TI_VAL_WRAP:
        return ti__wrap_field_thing(
                ((ti_wrap_t *) val)->thing,
                vp,
                *spec,
                options);
    case TI_VAL_ROOM:
        return ti_room_to_pk((ti_room_t *) val, &vp->pk, options);
    case TI_VAL_ARR:
    {
        ti_varr_t * varr = (ti_varr_t *) val;
        if (msgpack_pack_array(&vp->pk, varr->vec->n))
            return -1;
        for (vec_each(varr->vec, ti_val_t, v))
        {
            if (wrap__field_val(
                    t_field,
                    &t_field->nested_spec,
                    v,
                    vp,
                    options))
                return -1;
        }
        return 0;
    }
    case TI_VAL_SET:
        return wrap__set(
                (ti_vset_t *) val,
                vp,
                t_field->nested_spec,
                options);
    case TI_VAL_ERROR:
        return ti_verror_to_pk((ti_verror_t *) val, &vp->pk, options);
    case TI_VAL_MEMBER:
        return wrap__field_val(
                t_field,
                spec,
                VMEMBER(val),
                vp,
                options);
    case TI_VAL_MPDATA:
        return ti_raw_mpdata_to_pk((ti_raw_t *) val, &vp->pk, options);
    case TI_VAL_CLOSURE:
        return ti_closure_to_pk((ti_closure_t *) val, &vp->pk, options);
    case TI_VAL_FUTURE:
        return VFUT(val)
                ? wrap__field_val(
                        t_field,
                        spec,
                        VFUT(val),
                        vp,
                        options)
                : msgpack_pack_nil(&vp->pk);
    case TI_VAL_TEMPLATE:
        break;
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

int ti__wrap_methods_to_pk(
        ti_type_t * t_type,
        ti_thing_t * thing,
        ti_vp_t * vp,
        int options)
{
    int rc = 0;
    ex_t e = {0};
    ti_val_t * rval = vp->query->rval;
    uint8_t deep = vp->query->qbind.deep;

    for (vec_each(t_type->methods, ti_method_t, method))
    {
        vp->query->rval = NULL;
        vp->query->qbind.deep = (uint8_t) options;

        if (method->closure->flags & TI_CLOSURE_FLAG_WSE)
        {
            ex_set(&e, EX_BAD_DATA,
                    "failed to compute property; "
                    "method has side effects");
            vp->query->rval = (ti_val_t *) ti_verror_ensure_from_e(&e);
        }
        else if (ti_closure_call_one_arg(
                method->closure,
                vp->query,
                (ti_val_t *) thing,
                &e))
        {
            ti_val_gc_drop((ti_val_t *) vp->query->rval);
            vp->query->rval = (ti_val_t *) ti_verror_ensure_from_e(&e);
        }

        if (vp->query->futures.n)
        {
            link_clear(
                    &vp->query->futures,
                    (link_destroy_cb) ti_val_unsafe_drop);

            ti_val_gc_drop((ti_val_t *) vp->query->rval);

            ex_set(&e, EX_BAD_DATA,
                    "failed to compute property; "
                    "method contains futures");

            vp->query->rval = (ti_val_t *) ti_verror_ensure_from_e(&e);
        }

        rc = mp_pack_strn(
                &vp->pk,
                method->name->str,
                method->name->n) ||
            ti_val_to_pk(vp->query->rval, vp, vp->query->qbind.deep);

        ti_val_unsafe_gc_drop(vp->query->rval);

        if (rc)
            break;
    }

    vp->query->qbind.deep = deep;
    vp->query->rval = rval;
    return rc;
}

/*
 * Do not use directly, use ti_wrap_to_pk() instead
 */
int ti__wrap_field_thing(
        ti_thing_t * thing,
        ti_vp_t * vp,
        uint16_t spec,
        int options)
{
    size_t nm;
    ti_type_t * t_type;
    spec &= TI_SPEC_MASK_NILLABLE;

    assert (options >= 0);
    assert (thing->tp == TI_VAL_THING);
    assert (spec <= TI_SPEC_OBJECT);

    /*
     * Just return the ID when locked or if `options` (deep) has reached zero.
     */
    if ((thing->flags & TI_VFLAG_LOCK) || !options)
        return ti_thing_id_to_pk(thing, &vp->pk);

    /*
     * If `spec` is not a type or a none existing type (thus ANY or OBJECT),
     * then pack the thing as normal.
     */
    if (spec >= TI_SPEC_ANY ||  /* TI_SPEC_ANY || TI_SPEC_OBJECT */
        !(t_type = ti_types_by_id(thing->collection->types, spec)))
        return ti_thing__to_pk(thing, vp, options);

    /* decrement `options` (deep) by one */
    --options;

    /* Set the lock */
    thing->flags |= TI_VFLAG_LOCK;

    /* Number of methods to pack */
    nm = t_type->methods->n;

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
        size_t n = ti_min(t_type->fields->n, ti_thing_n(thing));
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
        if (wrap__thing_id_to_pk(thing, &vp->pk, n + nm))
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
                    &vp->pk,
                    map_get->prop->name->str,
                    map_get->prop->name->n) ||
                wrap__field_val(
                        map_get->field,
                        &map_get->field->spec,
                        map_get->prop->val,
                        vp,
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
        /*
         * Type mappings are only created the first time a conversion from
         * `to_type` -> `from_type` is asked so most likely the mappings are
         * returned from cache.
         */
        vec_t * mappings = ti_type_map(t_type, thing->via.type);
        if (!mappings || wrap__thing_id_to_pk(thing, &vp->pk, mappings->n + nm))
            goto fail;

        for (vec_each(mappings, ti_mapping_t, mapping))
        {
            if (mp_pack_strn(
                        &vp->pk,
                        mapping->f_field->name->str,
                        mapping->f_field->name->n) ||
                wrap__field_val(
                        mapping->t_field,
                        &mapping->t_field->spec,
                        VEC_get(thing->items.vec, mapping->f_field->idx),
                        vp,
                        options)
            ) goto fail;
        }
    }

    assert (vp->query != NULL);

    if (nm && ti__wrap_methods_to_pk(t_type, thing, vp, options))
        goto fail;

    thing->flags &= ~TI_VFLAG_LOCK;
    return 0;

fail:
    thing->flags &= ~TI_VFLAG_LOCK;
    return -1;
}

int ti_wrap_copy(ti_wrap_t ** wrap, uint8_t deep)
{
    assert (deep);
    ti_thing_t * other, * thing = (*wrap)->thing;
    ti_type_t * type = ti_types_by_id(
            thing->collection->types,
            (*wrap)->type_id);

    if (!type)
    {
        ti_incref(thing);

        if (ti_thing_copy(&thing, deep))
        {
            ti_decref(thing);
            return -1;
        }

        ti_val_unsafe_drop((ti_val_t *) *wrap);
        *wrap = (ti_wrap_t *) thing;
        return 0;
    }

    --deep;

    if (ti_thing_is_object(thing))
    {
        size_t n = ti_min(type->fields->n, ti_thing_n(thing));
        other = ti_thing_o_create(0, n , thing->collection);
        if (!other)
            return -1;

        for (vec_each(type->fields, ti_field_t, t_field))
        {
            ti_prop_t * prop, * p;
            prop = ti_thing_o_prop_weak_get(thing, t_field->name);
            if (!prop || !ti_field_maps_to_val(t_field, prop->val))
                continue;

            p = ti_prop_dup(prop);

            if (!p || ti_val_copy(&p->val, other, p->name, deep))
            {
                ti_prop_destroy(p);
                goto fail;
            }

            VEC_push(other->items.vec, p);
        };
    }
    else
    {
        vec_t * mappings = ti_type_map(type, thing->via.type);
        if (!mappings)
            return -1;

        other = ti_thing_o_create(0, mappings->n, thing->collection);
        if (!other)
            return -1;

        for (vec_each(mappings, ti_mapping_t, mapping))
        {
            ti_val_t * val = VEC_get(thing->items.vec, mapping->f_field->idx);
            ti_prop_t * p = ti_prop_create(mapping->f_field->name, val);
            if (!p)
                goto fail;

            ti_incref(p->name);
            ti_incref(p->val);

            if (ti_val_copy(&p->val, other, p->name, deep))
            {
                ti_prop_destroy(p);
                goto fail;
            }

            VEC_push(other->items.vec, p);
        }
    }

    ti_val_unsafe_drop((ti_val_t *) *wrap);
    *wrap = (ti_wrap_t *) other;
    return 0;

fail:
    ti_val_unsafe_drop((ti_val_t *) other);
    return -1;
}


int ti_wrap_dup(ti_wrap_t ** wrap, uint8_t deep)
{
    assert (deep);
    ti_wrap_t * nwrap = ti_wrap_create((*wrap)->thing, (*wrap)->type_id);
    if (!nwrap)
        return -1;

    if (ti_thing_dup(&nwrap->thing, deep))
    {
        ti_wrap_destroy(nwrap);
        return -1;
    }

    ti_val_unsafe_drop((ti_val_t *) *wrap);
    *wrap = nwrap;
    return 0;
}
