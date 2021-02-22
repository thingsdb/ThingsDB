/*
 * ti/condition.c
 */
#include <assert.h>
#include <doc.h>
#include <math.h>
#include <ti.h>
#include <ti/condition.h>
#include <ti/nil.h>
#include <ti/raw.inline.h>
#include <ti/spec.t.h>
#include <ti/val.inline.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <util/strx.h>

static ti_val_t * condition__dval_cb(ti_field_t * field)
{
    ti_val_t * dval = field->condition.none->dval;
    ti_incref(dval);
    return dval;
}


int ti_condition_field_range_init(
        ti_field_t * field,
        const char * str,
        size_t n,
        ex_t * e)
{
    ti_val_t * dval = NULL;
    ti_spec_enum_t spec;
    const char * end = str + n - 1;
    char * tmp;
    _Bool is_nillable = field->spec & TI_SPEC_NILLABLE;

    /* can be set even for nillable */
    field->dval_cb = condition__dval_cb;

    assert (*end == '>');

    switch(*str)
    {
    case 's':
        ++str;
        if (n > 3 && memcmp(str, "tr", 2) == 0)
        {
            spec = TI_SPEC_STR_RANGE;
            str += 2;
            break;
        }
        goto invalid;
    case 'i':
        ++str;
        if (n > 3 && memcmp(str, "nt", 2) == 0)
        {
            spec = TI_SPEC_INT_RANGE;
            str += 2;
            break;
        }
        goto invalid;
    case 'f':
        ++str;
        if (n > 5 && memcmp(str, "loat", 4) == 0)
        {
            spec = TI_SPEC_FLOAT_RANGE;
            str += 4;
            break;
        }
        goto invalid;
    default:
        goto invalid;
    }

    if (*str != '<')
    {
        ex_set(e, EX_VALUE_ERROR,
                "invalid declaration for `%s` on type `%s`; "
                "expecting a `<` character after `%.*s`"DOC_T_TYPE,
                field->name->str, field->type->name,
                (char *) str - (char *) field->spec_raw->data,
                (char *) field->spec_raw->data);
        return e->nr;
    }

    ++str;

    switch(spec)
    {
    case TI_SPEC_STR_RANGE:
    {
        int64_t ma, mi = strx_to_int64(str, &tmp);

        str = tmp;

        if (errno == ERANGE)
        {
            ex_set(e, EX_OVERFLOW, "integer overflow");
            return e->nr;
        }

        if (mi < 0)
        {
            ex_set(e, EX_VALUE_ERROR,
                    "invalid declaration for `%s` on type `%s`; "
                    "the minimum value for a string range must "
                    "not be negative"DOC_T_TYPE,
                    field->name->str, field->type->name);
            return e->nr;
        }

        if (*str != ':')
            goto missing_colon;

        ++str;

        ma = strx_to_int64(str, &tmp);

        str = tmp;

        if (errno == ERANGE)
        {
            ex_set(e, EX_OVERFLOW, "integer overflow");
            return e->nr;
        }

        if (ma < mi)
            goto smaller_max;

        if (*str == ':')
        {
            assert (end > str);
            ssize_t n = end - (++str);
            if (n < mi || n > ma)
                goto invalid_dval;

            dval = (ti_val_t *) ti_str_create(str, (size_t) n);
            str += n;
        }
        else if (is_nillable)
            dval = (ti_val_t *) ti_nil_get();
        else if (mi == 0)
            dval = (ti_val_t *) ti_val_empty_str();
        else
            dval = (ti_val_t *) ti_str_dval_n(mi);

        if (!dval)
        {
            ex_set_mem(e);
            return e->nr;
        }

        if (str != end)
            goto expect_end;

        field->condition.srange = malloc(sizeof(ti_condition_srange_t));
        if (!field->condition.srange)
        {
            ex_set_mem(e);
            goto failed;
        }

        field->condition.srange->dval = dval;
        field->condition.srange->mi = (size_t) mi;
        field->condition.srange->ma = (size_t) ma;
        field->spec |= spec;

        return 0;
    }
    case TI_SPEC_INT_RANGE:
    {
        int64_t dv, ma, mi = strx_to_int64(str, &tmp);

        str = tmp;

        if (errno == ERANGE)
        {
            ex_set(e, EX_OVERFLOW, "integer overflow");
            return e->nr;
        }

        if (*str != ':')
            goto missing_colon;

        ++str;

        ma = strx_to_int64(str, &tmp);

        str = tmp;

        if (errno == ERANGE)
        {
            ex_set(e, EX_OVERFLOW, "integer overflow");
            return e->nr;
        }

        if (ma < mi)
            goto smaller_max;

        if (*str == ':')
        {
            assert (end > str);
            dv = strx_to_int64(++str, &tmp);

            str = tmp;

            if (errno == ERANGE)
            {
                ex_set(e, EX_OVERFLOW, "integer overflow");
                return e->nr;
            }

            if (dv < mi || dv > ma)
                goto invalid_dval;

            dval = (ti_val_t *) ti_vint_create(dv);
        }
        else if (is_nillable)
            dval = (ti_val_t *) ti_nil_get();
        else
        {
            dv = mi < 0 && ma < 0
                    ? ma
                    : mi > 0 && ma > 0
                    ? mi
                    : 0;
            dval = (ti_val_t *) ti_vint_create(dv);
        }

        if (!dval)
        {
            ex_set_mem(e);
            return e->nr;
        }

        if (str != end)
            goto expect_end;

        field->condition.irange = malloc(sizeof(ti_condition_irange_t));
        if (!field->condition.irange)
        {
            ex_set_mem(e);
            goto failed;
        }

        field->condition.irange->dval = dval;
        field->condition.irange->mi = mi;
        field->condition.irange->ma = ma;
        field->spec |= spec;

        return 0;
    }
    case TI_SPEC_FLOAT_RANGE:
    {
        double dv, ma, mi = strx_to_double(str, &tmp);

        str = tmp;

        if (isnan(mi))
            goto nan_value;

        if (*str != ':')
            goto missing_colon;

        ++str;

        ma = strx_to_double(str, &tmp);

        str = tmp;

        if (isnan(ma))
            goto nan_value;

        if (ma < mi)
            goto smaller_max;

        if (*str == ':')
        {
            assert (end > str);
            dv = strx_to_double(++str, &tmp);

            str = tmp;

            if (isnan(dv))
                goto nan_value;

            if (dv < mi || dv > ma)
                goto invalid_dval;

            dval = (ti_val_t *) ti_vfloat_create(dv);
        }
        else if (is_nillable)
            dval = (ti_val_t *) ti_nil_get();
        else
        {
            dv = mi < 0.0 && ma < 0.0
                    ? ma
                    : mi > 0.0 && ma > 0.0
                    ? mi
                    : 0.0;
            dval = (ti_val_t *) ti_vfloat_create(dv);
        }

        if (!dval)
        {
            ex_set_mem(e);
            return e->nr;
        }

        if (str != end)
            goto expect_end;

        field->condition.drange = malloc(sizeof(ti_condition_drange_t));
        if (!field->condition.drange)
        {
            ex_set_mem(e);
            goto failed;
        }

        field->condition.drange->dval = dval;
        field->condition.drange->mi = mi;
        field->condition.drange->ma = ma;
        field->spec |= spec;

        return 0;
    }
    default:
        assert(0);
        ex_set_internal(e);
        goto failed;
    }


invalid:
    ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "range <..> conditions expect a minimum and maximum value and "
            "may only be applied to `int`, `float` or `str`"
            DOC_T_TYPE,
            field->name->str, field->type->name);
    goto failed;

missing_colon:
    ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "expecting a colon (:) after the minimum value of the range"
            DOC_T_TYPE,
            field->name->str, field->type->name);
    goto failed;

smaller_max:
    ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "expecting the maximum value to be greater than "
            "or equal to the minimum value"
            DOC_T_TYPE,
            field->name->str, field->type->name);
    goto failed;

expect_end:
    ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "expecting character `>` after `%.*s`"
            DOC_T_TYPE,
            field->name->str, field->type->name,
            (char *) str - (char *) field->spec_raw->data,
            (char *) field->spec_raw->data);
    goto failed;

nan_value:
    ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "not-a-number (nan) values are not allowed to specify a range"
            DOC_T_TYPE,
            field->name->str, field->type->name);
    goto failed;

invalid_dval:
    ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "the provided default value does not match the condition"
            DOC_T_TYPE,
            field->name->str, field->type->name);
    goto failed;

failed:
    ti_val_drop(dval);
    return e->nr;
}

int ti_condition_field_re_init(
        ti_field_t * field,
        const char * str,
        size_t n,
        ex_t * e)
{
    ti_regex_t * regex;
    ti_val_t * dval = NULL;
    const char * end = str + n - 1;
    _Bool is_nillable = field->spec & TI_SPEC_NILLABLE;

    /* can be set even for nillable */
    field->dval_cb = condition__dval_cb;

    assert (*str == '/');

    if (*end == '>')
    {
        for (size_t i = 0; !dval && end > str; ++i)
        {
            --end; --n;

            if (*end == '<')
            {
                dval = i
                        ? (ti_val_t *) ti_str_create(end+1, i)
                        : ti_val_empty_str();
                if (!dval)
                {
                    ex_set_mem(e);
                    return e->nr;
                }
                --end; --n;  /* end != `/` so we can decrement by one */
            }
        }

        if (!dval)
            goto invalid_syntax;
    }

    dval = dval
            ? dval
            : is_nillable
            ? (ti_val_t *) ti_nil_get()
            : ti_val_empty_str();

    if (*end == 'i')
        --end;  /* leave `n` since we want to include the `i` in the pattern */

    if (*end != '/')
        goto invalid_syntax;

    regex = ti_regex_from_strn(str, n, e);
    if (!regex)
        goto fail0;

    if (ti_val_is_str(dval) && !ti_regex_test(regex, (ti_raw_t *) dval))
    {
        ex_set(e, EX_VALUE_ERROR,
                "invalid declaration for `%s` on type `%s`; "
                "a non-nillable pattern requires a valid default value; "
                "for example: /(e|h|l|o)+/<hello>"
                DOC_T_TYPE,
                field->name->str, field->type->name);
        goto fail1;
    }

    field->condition.re = malloc(sizeof(ti_condition_re_t));
    if (!field->condition.re)
    {
        ex_set_mem(e);
        goto fail1;
    }

    field->condition.re->dval = dval;
    field->condition.re->regex = regex;
    field->spec |= TI_SPEC_REMATCH;

    return 0;

invalid_syntax:
    ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "pattern condition syntax is invalid"
            DOC_T_TYPE,
            field->name->str, field->type->name);
    goto fail0;

fail1:
    ti_val_drop((ti_val_t *) regex);
fail0:
    ti_val_drop(dval);
    return e->nr;
}

static void condition__del_type_cb(
        ti_field_t * field,
        ti_thing_t * thing,
        ti_thing_t * UNUSED(relation))
{
    ti_val_t ** vaddr = \
            (ti_val_t **) vec_get_addr(thing->items.vec, field->idx);
    ti_val_unsafe_gc_drop(*vaddr);
    *vaddr = (ti_val_t *) ti_nil_get();
}

static void condition__add_type_cb(
        ti_field_t * field,
        ti_thing_t * thing,
        ti_thing_t * relation)
{
    ti_val_t ** vaddr = \
            (ti_val_t **) vec_get_addr(thing->items.vec, field->idx);
    ti_val_unsafe_gc_drop(*vaddr);
    *vaddr = (ti_val_t *) relation;
    ti_incref(relation);
}

static void condition__del_set_cb(
        ti_field_t * field,
        ti_thing_t * thing,
        ti_thing_t * relation)
{
    ti_vset_t * vset = VEC_get(thing->items.vec, field->idx);
    ti_val_gc_drop(imap_pop(vset->imap, ti_thing_key(relation)));
}

static void condition__add_set_cb(
        ti_field_t * field,
        ti_thing_t * thing,
        ti_thing_t * relation)
{
    ti_vset_t * vset = VEC_get(thing->items.vec, field->idx);

    switch(imap_add(vset->imap, ti_thing_key(relation), relation))
    {
    case IMAP_SUCCESS:
        ti_incref(relation);
        return;
    case IMAP_ERR_EXIST:
        return;
    case IMAP_ERR_ALLOC:
        ti_panic("unrecoverable error");
    }
}

int ti_condition_field_rel_init(
        ti_field_t * field,
        ti_field_t * ofield,
        ex_t * e)
{
    ti_condition_rel_t * a, * b = malloc(sizeof(ti_condition_rel_t));

    a = (field == ofield) ? b : malloc(sizeof(ti_condition_rel_t));

    if (!a || !b)
        goto mem_error;

    a->field = ofield;
    a->del_cb = field->spec == TI_SPEC_SET
            ? condition__del_set_cb
            : condition__del_type_cb;
    a->add_cb = field->spec == TI_SPEC_SET
            ? condition__add_set_cb
            : condition__add_type_cb;
    field->condition.rel = a;

    b->field = field;
    b->del_cb = ofield->spec == TI_SPEC_SET
            ? condition__del_set_cb
            : condition__del_type_cb;
    b->add_cb = ofield->spec == TI_SPEC_SET
            ? condition__add_set_cb
            : condition__add_type_cb;
    ofield->condition.rel = b;

    return 0;

mem_error:
    free(b);
    ex_set_mem(e);
    return e->nr;
}

void ti_condition_destroy(ti_condition_via_t condition, uint16_t spec)
{
    if (!condition.none)
        return;

    spec = spec & TI_SPEC_MASK_NILLABLE;

    switch((ti_spec_enum_t) spec)
    {
    case TI_SPEC_ANY:
        return;  /* a field may be set to ANY while using mod_type in which
                    case a condition should be left alone */
    case TI_SPEC_REMATCH:
        ti_regex_destroy(condition.re->regex);
        /* fall through */
    case TI_SPEC_STR_RANGE:
    case TI_SPEC_INT_RANGE:
    case TI_SPEC_FLOAT_RANGE:
        ti_val_drop(condition.none->dval);
        /* fall through */
    default:
        free(condition.none);
        return;
    }
}
