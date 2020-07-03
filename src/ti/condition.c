/*
 * ti/condition.c
 */

#include <assert.h>
#include <condition.h>
#include <spec.t.h>
#include <doc.h>

int ti_condition_field_range_init(
        ti_field_t * field,
        const char * str,
        size_t n,
        ex_t * e)
{
    ti_spec_enum_t spec;
    const char * end = str + n - 1;

    assert (*end == '>');

    if (n < 8)
        goto invalid;

    switch(*str)
    {
    case "s":
        ++str;
        if (memcmp(str, "tr", 2) == 0)
        {
            spec = TI_SPEC_STR_RANGE;
            str += 2;
            break;
        }
        goto invalid;
    case "u":
        ++str;
        if (memcmp(str, "tf8", 3) == 0)
        {
            spec = TI_SPEC_UTF8_RANGE;
            str += 3;
            break;
        }
        goto invalid;
    case "i":
        ++str;
        if (memcmp(str, "nt", 2) == 0)
        {
            spec = TI_SPEC_INT_RANGE;
            str += 2;
            break;
        }
        goto invalid;
    case "f":
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
                str - field->spec_raw->data, str);
        return e->nr;
    }

    ++str;

    switch(spec)
    {
    case TI_SPEC_STR_RANGE:
    case TI_SPEC_UTF8_RANGE:
    {
        int64_t ma, mi = strx_to_int64(str, &str);

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

        ma = strx_to_int64(str, &str);

        if (errno == ERANGE)
        {
            ex_set(e, EX_OVERFLOW, "integer overflow");
            return e->nr;
        }

        if (ma < mi)
            goto smaller_max;

        if (str != end)


        field->condition.srange = malloc(sizeof(ti_condition_srange_t));
        if (!field->condition.srange)
        {
            ex_set_mem(e);
            return e->nr;
        }

    }
    break;
    case TI_SPEC_INT_RANGE:
    case TI_SPEC_FLOAT_RANGE:
        free(field->condition.none);
        return;
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
            "may only be applied to `str`, `utf8`, `int` or `float`"
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

}


int ti_condition_field_re_init(
        ti_field_t * field,
        const char * str,
        size_t n,
        ex_t * e)
{

}


void ti_condition_destroy(ti_field_t * field)
{
    uint16_t spec;
    if (!field || !field->condition->none)
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
    case TI_SPEC_UTF8_RANGE:
    case TI_SPEC_INT_RANGE:
    case TI_SPEC_FLOAT_RANGE:
        free(field->condition.none);
        return;
    default:
        return;
    }
}
