/*
 * ti/wrap.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/closure.h>
#include <ti/field.h>
#include <ti/mapping.h>
#include <ti/regex.h>
#include <ti/types.inline.h>
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
    ti_val_drop((ti_val_t *) wrap->thing);
    free(wrap);
}

static int wrap__set(
        ti_vset_t * vset,
        uint16_t * spec,
        qp_packer_t ** pckr,
        _Bool as_set,
        int options)
{
    vec_t * vec = imap_vec(vset->imap);
    if (!vec)
        return -1;

    if (as_set && (
            qp_add_map(pckr) ||
            qp_add_raw(*pckr, (const uchar * ) TI_KIND_S_SET, 1)
        ))
        return -1;

    if (qp_add_array(pckr))
        return -1;

    for (vec_each(vec, ti_thing_t, thing))
        if (ti__wrap_field_thing(
                *spec,
                thing,
                pckr,
                options))
            return -1;

    return qp_close_array(*pckr) || (as_set && qp_close_map(*pckr));
}

static int wrap__field_val(
        ti_field_t * t_field,
        uint16_t * spec,    /* points to t_field->spec or t_field->nested */
        ti_val_t * val,
        qp_packer_t ** pckr,
        int options)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:
        return qp_add_null(*pckr);
    case TI_VAL_INT:
        return qp_add_int(*pckr, ((ti_vint_t *) val)->int_);
    case TI_VAL_FLOAT:
        return qp_add_double(*pckr, ((ti_vfloat_t *) val)->float_);
    case TI_VAL_BOOL:
        return qp_add_bool(*pckr, ((ti_vbool_t *) val)->bool_);
    case TI_VAL_MP:
        return qp_add_qp(
                *pckr,
                ((ti_raw_t *) val)->data,
                ((ti_raw_t *) val)->n);
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
        /* TODO: correct */
        return qp_add_raw(
                *pckr,
                ((ti_raw_t *) val)->data,
                ((ti_raw_t *) val)->n);
    case TI_VAL_REGEX:
        return ti_regex_to_pk((ti_regex_t *) val, pckr);
    case TI_VAL_THING:
        return ti__wrap_field_thing(
                *spec,
                (ti_thing_t *) val,
                pckr,
                options);
    case TI_VAL_WRAP:
        return ti__wrap_field_thing(
                *spec,
                ((ti_wrap_t *) val)->thing,
                pckr,
                options);
    case TI_VAL_ARR:
        if (qp_add_array(pckr))
            return -1;
        for (vec_each(((ti_varr_t *) val)->vec, ti_val_t, v))
        {
            if (wrap__field_val(
                    t_field,
                    &t_field->nested_spec,
                    v,
                    pckr,
                    options))
                return -1;
        }
        return qp_close_array(*pckr);
    case TI_VAL_SET:
        return wrap__set(
                (ti_vset_t *) val,
                &t_field->nested_spec,
                pckr,
                (t_field->spec & TI_SPEC_MASK_NILLABLE) == TI_VAL_SET,
                options);
    case TI_VAL_CLOSURE:
        return ti_closure_to_pk((ti_closure_t *) val, pckr);
    case TI_VAL_ERROR:
        return ti_verror_to_pk((ti_verror_t *) val, pckr);
    }

    assert(0);
    return -1;
}

/*
 * Do not use directly, use ti_wrap_to_pk() instead
 */
int ti__wrap_field_thing(
        uint16_t spec,
        ti_thing_t * thing,
        qp_packer_t ** pckr,
        int options)
{
    ti_type_t * t_type;
    spec &= TI_SPEC_MASK_NILLABLE;

    assert (thing->tp == TI_VAL_THING);
    assert (spec <= TI_SPEC_OBJECT);

    if (    spec == TI_SPEC_ANY ||
            spec == TI_SPEC_OBJECT ||
            !(t_type = ti_types_by_id(thing->collection->types, spec)))
        return ti_thing_to_pk(thing, pckr, options);

    if (qp_add_map(pckr))
        return -1;

    if (thing->id && (
            qp_add_raw(*pckr, (const uchar *) TI_KIND_S_THING, 1) ||
            qp_add_int(*pckr, thing->id)))
        return -1;

    if ((thing->flags & TI_VFLAG_LOCK) || !options)
        goto stop;  /* no nesting */

    --options;

    thing->flags |= TI_VFLAG_LOCK;

    if (ti_thing_is_object(thing))
    {
        ti_prop_t * prop;

        for (vec_each(t_type->fields, ti_field_t, t_field))
        {
            prop = ti_thing_o_prop_weak_get(thing, t_field->name);
            if (!prop || !ti_field_maps_to_val(t_field, prop->val))
                continue;
            if (qp_add_raw(
                        *pckr,
                        (const uchar *) prop->name->str,
                        prop->name->n) ||
                wrap__field_val(
                        t_field,
                        &t_field->spec,
                        prop->val,
                        pckr,
                        options))
            {
                thing->flags &= ~TI_VFLAG_LOCK;
                return -1;
            }
        }
    }
    else
    {
        vec_t * mappings;
        ti_type_t * f_type = ti_thing_type(thing);

        mappings = ti_type_map(t_type, f_type);
        if (!mappings)
            return -1;

        for (vec_each(mappings, ti_mapping_t, mapping))
        {
            if (qp_add_raw(
                        *pckr,
                        (const uchar *) mapping->f_field->name->str,
                        mapping->f_field->name->n) ||
                wrap__field_val(
                        mapping->t_field,
                        &mapping->t_field->spec,
                        vec_get(thing->items, mapping->f_field->idx),
                        pckr,
                        options))
            {
                thing->flags &= ~TI_VFLAG_LOCK;
                return -1;
            }
        }
    }

    thing->flags &= ~TI_VFLAG_LOCK;

stop:
    return qp_close_map(*pckr);
}


