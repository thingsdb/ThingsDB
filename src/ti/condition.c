/*
 * ti/condition.c
 */

#include <assert.h>
#include <ti/condition.h>
#include <ti/spec.t.h>
#include <doc.h>
#include <util/strx.h>

int ti_condition_field_range_init(
        ti_field_t * field,
        const char * str,
        size_t n,
        ex_t * e)
{
    ti_spec_enum_t spec;
    const char * end = str + n - 1;
    char * tmp;

    assert (*end == '>');

    if (n < 8)
        goto invalid;

    switch(*str)
    {
    case 's':
        ++str;
        if (memcmp(str, "tr", 2) == 0)
        {
            spec = TI_SPEC_STR_RANGE;
            str += 2;
            break;
        }
        goto invalid;
    case 'i':
        ++str;
        if (memcmp(str, "nt", 2) == 0)
        {
            spec = TI_SPEC_INT_RANGE;
            str += 2;
            break;
        }
        goto invalid;
    case 'f':
        ++str;
        if (memcmp(str, "loat", 4) == 0)
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
        /* TODO: check error message */
        ex_set(e, EX_VALUE_ERROR,
                "invalid declaration for `%s` on type `%s`; "
                "expecting a `<` character after `%.*s`"DOC_T_TYPE,
                field->name->str, field->type->name,
                (char *) str - (char *) field->spec_raw->data, str);
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
            /* TODO: check error message */
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

        if (errno == ERANGE)
        {
            ex_set(e, EX_OVERFLOW, "integer overflow");
            return e->nr;
        }

        if (ma < mi)
            goto smaller_max;

        if (str != end)
            goto expect_end;

        field->condition.srange = malloc(sizeof(ti_condition_srange_t));
        if (!field->condition.srange)
        {
            ex_set_mem(e);
            return e->nr;
        }

        field->condition.srange->mi = (size_t) mi;
        field->condition.srange->ma = (size_t) ma;

        return 0;
    }
    case TI_SPEC_INT_RANGE:
    {
        int64_t ma, mi = strx_to_int64(str, &tmp);

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

        if (str != end)
            goto expect_end;

        field->condition.srange = malloc(sizeof(ti_condition_irange_t));
        if (!field->condition.srange)
        {
            ex_set_mem(e);
            return e->nr;
        }

        field->condition.irange->mi = mi;
        field->condition.irange->ma = ma;

        return 0;
    }
    case TI_SPEC_FLOAT_RANGE:
    {
        double ma, mi = strx_to_double(str, &tmp);

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

        if (str != end)
            goto expect_end;

        field->condition.drange = malloc(sizeof(ti_condition_drange_t));
        if (!field->condition.drange)
        {
            ex_set_mem(e);
            return e->nr;
        }

        field->condition.drange->mi = mi;
        field->condition.drange->ma = ma;

        return 0;
    }
    default:
        assert(0);
        ex_set_internal(e);
        return e->nr;;
    }


invalid:
    /* TODO: check error message */
    ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "range <..> conditions expect a minimum and maximum value and "
            "may only be applied to `int`, `float` or `str`"
            DOC_T_TYPE,
            field->name->str, field->type->name);
    return e->nr;

missing_colon:
    /* TODO: check error message */
    ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "expecting a colon (:) after the minimum value of the range"
            DOC_T_TYPE,
            field->name->str, field->type->name);
    return e->nr;

smaller_max:
    /* TODO: check error message */
    ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "expecting the maximum value to be greater than "
            "or equal to the minimum value"
            DOC_T_TYPE,
            field->name->str, field->type->name);
    return e->nr;

expect_end:
    /* TODO: check error message */
    ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "expecting `>` after the maximum value of the range"
            DOC_T_TYPE,
            field->name->str, field->type->name);
    return e->nr;

nan_value:
    /* TODO: check error message */
    ex_set(e, EX_VALUE_ERROR,
            "invalid declaration for `%s` on type `%s`; "
            "not-a-number (nan) values are not allowed to specify a range"
            DOC_T_TYPE,
            field->name->str, field->type->name);
    return e->nr;
}

int ti_condition_field_re_init(
        ti_field_t * field,
        const char * str,
        size_t n,
        ex_t * e)
{
    return 0;
}


void ti_condition_destroy(ti_field_t * field)
{
    uint16_t spec;
    if (!field || !field->condition.none)
        return;

    spec = field->spec & TI_SPEC_MASK_NILLABLE;

    switch((ti_spec_enum_t) spec)
    {
    case TI_SPEC_REMATCH:
        pcre2_match_data_free(field->condition.re->match_data);
        pcre2_code_free(field->condition.re->code);
        free(field->condition.re);
        return;
    case TI_SPEC_STR_RANGE:
    case TI_SPEC_INT_RANGE:
    case TI_SPEC_FLOAT_RANGE:
        free(field->condition.none);
        return;
    default:
        return;
    }
}
