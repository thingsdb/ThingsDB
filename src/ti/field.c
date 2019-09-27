/*
 * ti/field.c
 */
#include <assert.h>
#include <doc.h>
#include <stdlib.h>
#include <ti/field.h>
#include <ti/vint.h>
#include <ti/vset.h>
#include <ti/thingi.h>
#include <ti/varr.h>
#include <ti/typesi.h>
#include <ti/spec.h>
#include <ti/speci.h>
#include <ti/names.h>
#include <util/strx.h>


static _Bool field__spec_is_ascii(
        ti_field_t * field,
        const char * str,
        size_t n,
        ex_t * e)
{
    if (!strx_is_asciin(str, n))
    {
        ex_set(e, EX_VALUE_ERROR,
                "invalid declaration for `%s` on type `%s`; "
                "type declarations must only contain valid ASCII characters"
                DOC_SPEC,
                field->name->str, field->type->name);
        return false;
    }
    return true;
}

static inline _Bool field__cmp(const char * a, size_t na, const char * str)
{
    return strlen(str) == na && memcmp(a, str, na) == 0;
}

static int field__init(ti_field_t * field, ex_t * e)
{
    const char * str = (const char *) field->spec_raw->data;
    size_t n = field->spec_raw->n;
    uint16_t * spec = &field->spec;

    field->spec = 0;
    field->nested_spec = 0;

    if (!n)
    {
        ex_set(e, EX_VALUE_ERROR,
                "invalid declaration for `%s` on type `%s`; "
                "type declarations must not be empty"DOC_SPEC,
                field->name->str, field->type->name);
        return e->nr;
    }

    if (str[n-1] == '?')
    {
        field->spec |= TI_SPEC_NILLABLE;
        if (!--n)
            goto invalid;
    }

    if (*str == '[')
    {
        if (str[n-1] != ']')
            goto invalid;
        field->spec |= TI_SPEC_ARR;
    }
    else if (*str == '{')
    {
        if (str[n-1] != '}')
            goto invalid;
        field->spec |= TI_SPEC_SET;
    }
    else
    {
        field->nested_spec = TI_SPEC_ANY;  /* must default to any */
        goto skip_nesting;
    }

    /* continue array or set definition */

    if (!(n -= 2))
    {
        field->nested_spec = TI_SPEC_ANY;  /* must default to any */
        return 0;
    }

    spec = &field->nested_spec;
    ++str;

    if (str[n-1] == '?')
    {
        *spec |= TI_SPEC_NILLABLE;
        if (!--n)
            goto invalid;
    }

skip_nesting:

    assert (n);

    if (*str == '[')
    {
        ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "unexpected `[`; nested array declarations are not allowed"
            DOC_SPEC, field->name->str, field->type->name);
        return e->nr;
    }

    if (*str == '{')
    {
        ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "unexpected `{`; nested set declarations are not allowed"
            DOC_SPEC, field->name->str, field->type->name);
        return e->nr;
    }

    if (field__cmp(str, n, "any"))
    {
        *spec |= TI_SPEC_ANY;
    }
    else if (field__cmp(str, n, TI_VAL_THING_S))
    {
        *spec |= TI_SPEC_OBJECT;
    }
    else if (field__cmp(str, n, TI_VAL_RAW_S))
    {
        *spec |= TI_SPEC_RAW;
    }
    else if (field__cmp(str, n, "str") || field__cmp(str, n, "utf8"))
    {
        *spec |= TI_SPEC_UTF8;
    }
    else if (field__cmp(str, n, TI_VAL_INT_S))
    {
        *spec |= TI_SPEC_INT;
    }
    else if (field__cmp(str, n, "uint"))
    {
        *spec |= TI_SPEC_UINT;
    }
    else if (field__cmp(str, n, TI_VAL_FLOAT_S))
    {
        *spec |= TI_SPEC_FLOAT;
    }
    else if (field__cmp(str, n, "number"))
    {
        *spec |= TI_SPEC_NUMBER;
    }
    else if (field__cmp(str, n, TI_VAL_BOOL_S))
    {
        *spec |= TI_SPEC_BOOL;
    }
    else if (field__cmp(str, n, field->type->name))
    {
        *spec = field->type->type_id;
        if (&field->spec == spec && (~field->spec & TI_SPEC_MASK_NILLABLE))
        {
            ex_set(e, EX_VALUE_ERROR,
                "invalid declaration for `%s` on type `%s`; "
                "missing `?` after declaration `%.*s`; "
                "self references must be nillable"
                DOC_SPEC,
                field->name->str, field->type->name,
                (int) n, str);
            return e->nr;
        }
    }
    else
    {
        ti_type_t * dep = smap_getn(field->type->types->smap, str, n);
        if (!dep)
        {
            if (field__spec_is_ascii(field,str, n, e))
                ex_set(e, EX_TYPE_ERROR,
                        "invalid declaration for `%s` on type `%s`; "
                        "unknown type `%.*s` in declaration"DOC_SPEC,
                        field->name->str, field->type->name,
                        (int) n, str);
            return e->nr;
        }

        *spec |= dep->type_id;

        if (vec_push(&field->type->dependencies, dep))
        {
            ex_set_mem(e);
            return e->nr;
        }

        ++dep->refcount;
    }

    if ((field->spec & TI_SPEC_MASK_NILLABLE) == TI_SPEC_SET &&
        (field->nested_spec & TI_SPEC_MASK_NILLABLE) > TI_SPEC_OBJECT)
    {
        ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "type `"TI_VAL_SET_S"` cannot contain type `%s`"
            DOC_SPEC,
            field->name->str, field->type->name,
            ti__spec_approx_type_str(field->nested_spec));
        return e->nr;
    }

    if (((*spec) & TI_SPEC_MASK_NILLABLE) == TI_SPEC_ANY)
        *spec &= TI_SPEC_MASK_NILLABLE;

    return 0;

invalid:
    if (!field__spec_is_ascii(
            field,
            (const char *) field->spec_raw->data,
            field->spec_raw->n,
            e))
        return e->nr;

    ex_set(e, EX_VALUE_ERROR,
        "invalid declaration for `%s` on type `%s`; "
        "expecting a valid type declaration but got `%.*s` instead"
        DOC_SPEC,
        field->name->str, field->type->name,
        (int) field->spec_raw->n,
        (const char *) field->spec_raw->data);

    return e->nr;
}

int ti_field_create(
        ti_name_t * name,
        ti_raw_t * spec_raw,
        ti_type_t * type,
        ex_t * e)
{
    ti_field_t * field = malloc(sizeof(ti_field_t));
    if (!field)
    {
        ex_set_mem(e);
        return e->nr;
    }

    field->type = type;
    field->name = name;
    field->spec_raw = spec_raw;
    field->idx = type->fields->n;

    ti_incref(name);
    ti_incref(spec_raw);

    if (field__init(field, e))
    {
        assert (e->nr);
        ti_field_destroy(field);
        return e->nr;
    }

    if (vec_push(&type->fields, field))
    {
        ex_set_mem(e);
        ti_field_destroy(field);
    }

    return e->nr;
}


void ti_field_destroy(ti_field_t * field)
{
    if (!field)
        return;

    ti_name_drop(field->name);
    ti_val_drop((ti_val_t *) field->spec_raw);

    free(field);
}

static int field__vset_assign(ti_field_t * field, ti_vset_t ** vset, ex_t * e)
{
    vec_t * vec;

    if (field->nested_spec == TI_SPEC_ANY ||
        field->nested_spec == (*vset)->spec ||
        (*vset)->imap->n == 0)
        goto done;

    vec = imap_vec((*vset)->imap);
    if (!vec)
    {
        ex_set_mem(e);
        return e->nr;
    }

    for (vec_each(vec, ti_thing_t, thing))
    {
        /* sets cannot hold type `nil` so we can ignore the nillable flag */
        if (thing->type_id != (*vset)->spec)
        {
            ex_set(e, EX_TYPE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` has definition `%.*s` but got a set with "
                "type `%s` instead",
                field->type->name,
                field->name->str,
                (int) field->spec_raw->n,
                (const char *) field->spec_raw->data,
                ti_thing_type_str(thing));
            return e->nr;
        }
    }

done:
    if (ti_val_make_assignable((ti_val_t **) vset, e) == 0)
        (*vset)->spec = field->nested_spec;
    return e->nr;
}

static int field__varr_assign(ti_field_t * field, ti_varr_t ** varr, ex_t * e)
{
    if (field->nested_spec == TI_SPEC_ANY ||
        (*varr)->vec->n == 0 ||
        (*varr)->spec == field->nested_spec)
        goto done;

    for (vec_each((*varr)->vec, ti_val_t, val))
    {
        switch (ti_spec_check_val(field->nested_spec, val))
        {
        case TI_SPEC_RVAL_SUCCESS:
            continue;
        case TI_SPEC_RVAL_TYPE_ERROR:
            ex_set(e, EX_TYPE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` requires an array with items that match "
                "definition `%.*s`",
                field->type->name,
                field->name->str,
                (int) field->spec_raw->n,
                (const char *) field->spec_raw->data);
            return e->nr;
        case TI_SPEC_RVAL_UTF8_ERROR:
            ex_set(e, EX_VALUE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` requires an array with UTF8 string values",
                field->type->name,
                field->name->str);
            return e->nr;
        case TI_SPEC_RVAL_UINT_ERROR:
            ex_set(e, EX_VALUE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` requires an array with positive integer values",
                field->type->name,
                field->name->str);
            return e->nr;
        }
    }

done:
    if (ti_val_make_assignable((ti_val_t **) varr, e) == 0)
        (*varr)->spec = field->nested_spec;
    return e->nr;
}

/*
 * Returns 0 if the given value is valid for this field
 */
int ti_field_make_assignable(ti_field_t * field, ti_val_t ** val, ex_t * e)
{
    switch (ti_spec_check_val(field->spec, *val))
    {
    case TI_SPEC_RVAL_SUCCESS:
        return ti_val_is_array(*val)
                ? field__varr_assign(field, (ti_varr_t **) val, e)
                : ti_val_is_set(*val)
                ? field__vset_assign(field, (ti_vset_t **) val, e)
                : ti_val_make_assignable(val, e);
    case TI_SPEC_RVAL_TYPE_ERROR:
        ex_set(e, EX_TYPE_ERROR,
                "mismatch in type `%s`; "
                "type `%s` is invalid for property `%s` with definition `%.*s`",
                field->type->name,
                ti_val_str(*val),
                field->name->str,
                (int) field->spec_raw->n,
                (const char *) field->spec_raw->data);
        break;
    case TI_SPEC_RVAL_UTF8_ERROR:
        ex_set(e, EX_VALUE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` only accepts valid UTF8 data",
                field->type->name,
                field->name->str);
        break;
    case TI_SPEC_RVAL_UINT_ERROR:
        ex_set(e, EX_VALUE_ERROR,
                "mismatch in type `%s`; "
                "property `%s` only accepts positive integer values",
                field->type->name,
                field->name->str);
        break;
    }
    return e->nr;
}

static inline int field__cast_err(
        ti_field_t * t_field,
        ti_field_t * f_field,
        ex_t * e)
{
    ex_set(e, EX_TYPE_ERROR,
            "failed casting property `%s`; "
            "type definition `%.*s` cannot cast to `%.*s`",
            t_field->name->str,
            (int) f_field->spec_raw->n,
            (const char *) f_field->spec_raw->data,
            (int) t_field->spec_raw->n,
            (const char *) t_field->spec_raw->data);
    return e->nr;
}

static inline int field__nil_err(
        ti_field_t * t_field,
        ti_field_t * f_field,
        ex_t * e)
{
    ex_set(e, EX_TYPE_ERROR,
            "failed casting property `%s`; "
            "cannot cast nillable definition `%.*s` to "
            "non-nillable definition `%.*s`",
            t_field->name->str,
            (int) f_field->spec_raw->n,
            (const char *) f_field->spec_raw->data,
            (int) t_field->spec_raw->n,
            (const char *) t_field->spec_raw->data);
    return e->nr;
}

static int field__check_spec(
        ti_field_t * t_field,
        ti_field_t * f_field,
        uint16_t t_spec,
        uint16_t f_spec,
        ex_t * e)
{
    switch ((ti_spec_enum_t) t_spec)
    {
    case TI_SPEC_ANY:
        return 0;       /* already checked */
    case TI_SPEC_OBJECT:
        return f_spec < TI_SPEC_ANY
                ? 0
                : field__cast_err(t_field, f_field, e);
    case TI_SPEC_RAW:
        return f_spec == TI_SPEC_UTF8
                ? 0
                : field__cast_err(t_field, f_field, e);
    case TI_SPEC_INT:
        return f_spec == TI_SPEC_UINT
                ? 0
                : field__cast_err(t_field, f_field, e);
    case TI_SPEC_NUMBER:
        return (f_spec == TI_SPEC_INT ||
                f_spec == TI_SPEC_UINT ||
                f_spec == TI_SPEC_FLOAT)
                ? 0
                : field__cast_err(t_field, f_field, e);
    case TI_SPEC_UTF8:
    case TI_SPEC_UINT:
    case TI_SPEC_FLOAT:
    case TI_SPEC_BOOL:
    case TI_SPEC_ARR:
    case TI_SPEC_SET:
        return field__cast_err(t_field, f_field, e);
    }

    if (f_spec < TI_SPEC_ANY)
    {
        /* we are left with two type which might be able to cast */
        assert (t_spec != f_spec);
        assert (t_spec < TI_SPEC_ANY);
        assert (f_spec < TI_SPEC_ANY);

        ti_type_t * t_type = ti_types_by_id(t_field->type->types, t_spec);
        ti_type_t * f_type = ti_types_by_id(f_field->type->types, t_spec);

        (void) ti_type_map(t_type, f_type, e);
        return e->nr;
    }
    return field__cast_err(t_field, f_field, e);
}

static int field__check_nested(
        ti_field_t * t_field,
        ti_field_t * f_field,
        ex_t * e)
{
    uint16_t t_spec, f_spec;

    assert ((t_field->spec & TI_SPEC_MASK_NILLABLE) ==
            (f_field->spec & TI_SPEC_MASK_NILLABLE));
    assert (t_field->nested_spec != TI_SPEC_ANY);

    if (    (~t_field->nested_spec & TI_SPEC_NILLABLE) &&
            (f_field->nested_spec & TI_SPEC_NILLABLE))
        return field__nil_err(t_field, f_field, e);

    t_spec = t_field->nested_spec & TI_SPEC_MASK_NILLABLE;
    f_spec = f_field->nested_spec & TI_SPEC_MASK_NILLABLE;

    if (t_spec == f_spec)
        return 0;

    return field__check_spec(t_field, f_field, t_spec, f_spec, e);
}

int ti_field_check_field(ti_field_t * t_field, ti_field_t * f_field, ex_t * e)
{
    uint16_t t_spec, f_spec;
    assert (t_field->name == f_field->name);

    /* return 0 when `to` accepts `any` (which is never set with nillable) */
    if (t_field->spec == TI_SPEC_ANY)
        return 0;

    /* if `to` does not accept `nil`, and from does, this is an error */
    if (    (~t_field->spec & TI_SPEC_NILLABLE) &&
            (f_field->spec & TI_SPEC_NILLABLE))
        return field__nil_err(t_field, f_field, e);

    t_spec = t_field->spec & TI_SPEC_MASK_NILLABLE;
    f_spec = f_field->spec & TI_SPEC_MASK_NILLABLE;

    /* return 0 when both specifications are equal, and nested accepts
     * anything which is default for all other than `arr` and `set` */
    return t_spec == f_spec
            ? t_field->nested_spec == TI_SPEC_ANY
            ? 0
            : field__check_nested(t_field, f_field, e)
            : field__check_spec(t_field, f_field, t_spec, f_spec, e);
}

ti_field_t * ti_field_by_name(ti_type_t * type, ti_name_t * name)
{
    for (vec_each(type->fields, ti_field_t, field))
        if (field->name == name)
            return field;
    return NULL;
}

ti_field_t * ti_field_by_strn_e(
        ti_type_t * type,
        const char * str,
        size_t n,
        ex_t * e)
{
    ti_name_t * name = ti_names_weak_get(str, n);
    if (name)
        for (vec_each(type->fields, ti_field_t, field))
            if (field->name == name)
                return field;

    if (ti_name_is_valid_strn(str, n))
        ex_set(e, EX_LOOKUP_ERROR, "type `%s` has no property `%.*s",
                type->name,
                (int) n, str);
    else
        ex_set(e, EX_VALUE_ERROR,
                "property name must follow the naming rules"DOC_NAMES);

    return NULL;
}
