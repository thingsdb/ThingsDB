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
#include <ti/spec.h>
#include <ti/speci.h>
#include <util/strx.h>


static _Bool field__spec_is_ascii(
        ti_field_t * field,
        ti_type_t * type,
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
                field->name->str, type->name);
        return false;
    }
    return true;
}

static inline _Bool field__cmp(const char * a, size_t na, const char * str)
{
    return strlen(str) == na && memcmp(a, str, na) == 0;
}

static int field__init(
        ti_field_t * field,
        ti_type_t * type,
        ti_types_t * types,
        ex_t * e)
{
    const char * str = (const char *) field->spec_raw->data;
    size_t n = field->spec_raw->n;
    uint16_t * spec = &field->spec;

    field->spec = 0;
    field->nested_spec = TI_SPEC_ANY;

    if (!n)
    {
        ex_set(e, EX_VALUE_ERROR,
                "invalid declaration for `%s` on type `%s`; "
                "type declarations must not be empty"DOC_SPEC,
                field->name->str, type->name);
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
    else goto skip_nesting;

    /* continue array or set definition */

    if (!(n -= 2))
        return 0;

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
            DOC_SPEC, field->name->str, type->name);
        return e->nr;
    }

    if (*str == '{')
    {
        ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "unexpected `{`; nested set declarations are not allowed"
            DOC_SPEC, field->name->str, type->name);
        return e->nr;
    }

    if (field__cmp(str, n, TI_VAL_THING_S))
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
    else if (field__cmp(str, n, type->name))
    {
        *spec |= type->type_id;
        if (&field->spec == spec && (~field->spec & TI_SPEC_MASK_NILLABLE))
        {
            ex_set(e, EX_VALUE_ERROR,
                "invalid declaration for `%s` on type `%s`; "
                "missing `?` after declaration `%.*s`; "
                "self references must be nillable"
                DOC_SPEC,
                field->name->str, type->name,
                (int) n, str);
            return e->nr;
        }
    }
    else
    {
        ti_type_t * dep = smap_getn(types->smap, str, n);
        if (!dep)
        {
            if (field__spec_is_ascii(field, type, str, n, e))
                ex_set(e, EX_TYPE_ERROR,
                        "invalid declaration for `%s` on type `%s`; "
                        "unknown type `%.*s` in declaration"DOC_SPEC,
                        field->name->str, type->name,
                        (int) n, str);
            return e->nr;
        }

        *spec |= dep->type_id;

        if (vec_push(&type->dependencies, dep))
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
            field->name->str, type->name,
            ti__spec_approx_type_str(field->nested_spec));
        return e->nr;
    }

    return 0;

invalid:
    if (field__spec_is_ascii(
            field, type,
            (const char *) field->spec_raw->data,
            field->spec_raw->n,
            e))
        return e->nr;

    ex_set(e, EX_VALUE_ERROR,
        "invalid declaration for `%s` on type `%s`; "
        "expecting a valid type declaration but got `%.*s` instead"
        DOC_SPEC,
        field->name->str, type->name,
        (int) field->spec_raw->n,
        (const char *) field->spec_raw->data);

    return e->nr;
}

int ti_field_create(
        ti_name_t * name,
        ti_raw_t * spec_raw,
        ti_type_t * type,
        ti_types_t * types,
        ex_t * e)
{
    ti_field_t * field = malloc(sizeof(ti_field_t));
    if (!field)
    {
        ex_set_mem(e);
        return e->nr;
    }

    field->name = name;
    field->spec_raw = spec_raw;

    ti_incref(name);
    ti_incref(spec_raw);

    if (field__init(field, type, types, e))
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

static int field__vset_check(ti_field_t * field, ti_vset_t * vset, ex_t * e)
{
    vec_t * vec;

    if (field->nested_spec == TI_SPEC_ANY ||
        field->nested_spec == vset->spec)
        return 0;

    if (vset->imap->n == 0)
        goto done;

    vec = imap_vec(vset->imap);
    if (!vec)
    {
        ex_set_mem(e);
        return e->nr;
    }

    for (vec_each(vec, ti_thing_t, thing))
    {
        /* sets cannot hold type `nil` so we can ignore the nillable flag */
        if (thing->type_id != vset->spec)
        {
            ex_set(e, EX_TYPE_ERROR,
                "property `%s` has definition `%.*s` but got a set with "
                "type `%s` instead",
                field->name->str,
                (int) field->spec_raw->n,
                (const char *) field->spec_raw->data,
                ti_thing_type_str(thing));
            return e->nr;
        }
    }

done:
    vset->spec = field->nested_spec;
    return 0;
}

static int field__varr_check(ti_field_t * field, ti_varr_t * varr, ex_t * e)
{
    if (field->nested_spec == TI_SPEC_ANY ||
        varr->vec->n == 0 ||
        varr->spec == field->nested_spec)
        return 0;

    for (vec_each(varr->vec, ti_val_t, val))
    {
        switch (ti_spec_check_val(varr->spec, val))
        {
        case TI_SPEC_RVAL_SUCCESS:
            continue;
        case TI_SPEC_RVAL_TYPE_ERROR:
            ex_set(e, EX_TYPE_ERROR,
                "property `%s` requires an array with items that match "
                "definition `%.*s`",
                field->name->str,
                (int) field->spec_raw->n,
                (const char *) field->spec_raw->data);
            return e->nr;
        case TI_SPEC_RVAL_UTF8_ERROR:
            ex_set(e, EX_VALUE_ERROR,
                "property `%s` requires an array with UTF8 string values",
                field->name->str);
            return e->nr;
        case TI_SPEC_RVAL_UINT_ERROR:
            ex_set(e, EX_VALUE_ERROR,
                "property `%s` requires an array with positive integer values",
                field->name->str);
            return e->nr;
        }
    }
    return 0;
}

/*
 * Returns 0 if the given value is valid for this field
 */
int ti_field_check_val(ti_field_t * field, ti_val_t * val, ex_t * e)
{
    if (!val)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "property `%s` is missing",
                field->name->str);
        return e->nr;
    }

    switch (ti_spec_check_val(field->spec, val))
    {
    case TI_SPEC_RVAL_SUCCESS:
        return ti_val_is_array(val)
                ? field__varr_check(field, ((ti_varr_t *) val), e)
                : ti_val_is_set(val)
                ? field__vset_check(field, ((ti_vset_t *) val), e)
                : 0;
    case TI_SPEC_RVAL_TYPE_ERROR:
        ex_set(e, EX_TYPE_ERROR,
                "type `%s` is invalid for property `%s` with definition `%.*s`",
                ti_val_str(val),
                field->name->str,
                (int) field->spec_raw->n,
                (const char *) field->spec_raw->data);
        break;
    case TI_SPEC_RVAL_UTF8_ERROR:
        ex_set(e, EX_VALUE_ERROR,
                "property `%s` only accepts valid UTF8 data",
                field->name->str);
        break;
    case TI_SPEC_RVAL_UINT_ERROR:
        ex_set(e, EX_VALUE_ERROR,
                "property `%s` only accepts positive integer values",
                field->name->str);
        break;
    }
    return e->nr;
}

int ti_spec_check_map(uint16_t to, uint16_t from)
{
    if ((to & TI_SPEC_MASK_NILLABLE) == TI_SPEC_ANY)
        return 0;
    if ((~to & TI_SPEC_NILLABLE) && (from & TI_SPEC_NILLABLE))
    {
        return
    }
}

int ti_field_check_field(ti_field_t * to, ti_field_t * from, ex_t * e)
{
    assert (to->name == from->name);
    if (to->spec == TI_SPEC_ANY)
        return 0;
    if (from->spec & TI_SPEC_NILLABLE)
}
