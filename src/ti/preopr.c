/*
 * ti/preopr.c
 */
#include <assert.h>
#include <ctype.h>
#include <ti/preopr.h>
#include <ti/val.inline.h>
#include <ti/vbool.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <util/logger.h>

enum
{
    PO__FALSE,
    PO__BOOL,
    PO__NEGATIVE,
    PO__AS_NUM,
    PO__CHK_NUM,
    PO__BIT_INV,
    PO__BITWISE,
};

enum
{
    PO__FLAG_FALSE    =1<<PO__FALSE,
    PO__FLAG_BOOL     =1<<PO__BOOL,
    PO__FLAG_NEGATIVE =1<<PO__NEGATIVE,
    PO__FLAG_AS_NUM   =1<<PO__AS_NUM,
    PO__FLAG_CHK_NUM  =1<<PO__CHK_NUM,
    PO__FLAG_BIT_INV  =1<<PO__BIT_INV,
    PO__FLAG_BITWISE  =1<<PO__BITWISE,
};

int ti_preopr_bind(const char * s, size_t n)
{
    register int preopr = 0;
    register int negative = 0;
    register int nots = 0;
    register int inverts = 0;

    if (!n)
        return preopr;  /* return 0 */

    assert(!isspace(*s));  /* white space is allowed, but not at the
                               beginning or end */

    switch(*s)
    {
    case '-':
        ++negative;
        /* fall through */
    case '+':
        preopr |= PO__FLAG_AS_NUM;
        break;
    case '!':
        ++nots;
        break;
    case '~':
        ++inverts;
        preopr |= PO__FLAG_AS_NUM|PO__FLAG_BITWISE;
    }

    while (--n)
    {
        switch(*(++s))
        {
        case '-':
            negative += !nots;  /* negative signs on the right of a ! have no
                                   logic meaning and can be ignored */
            break;
        case '!':
            ++nots;
            break;
        case '~':
            ++inverts;
        }
    }

    assert(!isspace(*s));  /* white space is allowed, but not at the
                               beginning or end */

    preopr |= (
        ((nots & 1)     << PO__FALSE) |
        ((!!nots)       << PO__BOOL) |
        ((negative & 1) << PO__NEGATIVE) |
        ((*s != '!')    << PO__CHK_NUM) |
        ((inverts & 1)  << PO__BIT_INV)
    );

    return preopr;
}

int ti_preopr_calc(int preopr, ti_val_t ** val, ex_t * e)
{
    ti_val_t * v = *val;

    /*
     * Flag PO__FLAG_CHK_NUM is set if a sequence of +- and ! ends with a
     * plus or minus sign. In this case the variable needs to be checked if
     * it supports a plus or minus sign.
     */
    if ((preopr & PO__FLAG_CHK_NUM) &&
        v->tp != TI_VAL_INT &&
        v->tp != TI_VAL_FLOAT &&
        v->tp != TI_VAL_BOOL)
    {
        ex_set(e, EX_TYPE_ERROR,
                "`%s` not supported by type `%s`",
                (preopr & PO__FLAG_BITWISE) ? "~" : "-/+",
                ti_val_str(v));
        return e->nr;
    }

    /*
     * Flag PO__FLAG_BOOL is set if one or more not signs (!) are used in
     * the sequence, meaning that the end value if one of -1, 0, 1, true or
     * false.
     */
    if (preopr & PO__FLAG_BOOL)
    {
        _Bool b = (preopr & PO__FLAG_FALSE) ^ ti_val_as_bool(v);
        ti_val_unsafe_drop(v);
        *val = (preopr & PO__FLAG_BIT_INV)
            ? (ti_val_t *) ti_vint_create(preopr & PO__FLAG_NEGATIVE ? ~((int) -b) : ~((int) b))
            : (preopr & PO__FLAG_AS_NUM)
            ? (ti_val_t *) ti_vint_create(preopr & PO__FLAG_NEGATIVE ? -b : b)
            : (ti_val_t *) ti_vbool_get(b);

        return 0;
    }

    if (preopr & PO__FLAG_BITWISE) switch(v->tp)
    {
    case TI_VAL_INT:
        if (preopr & PO__FLAG_BIT_INV)
        {
            int64_t i = VINT(v);
            if (preopr & PO__FLAG_NEGATIVE)
            {
                if (i == LLONG_MIN)
                {
                    ex_set(e, EX_OVERFLOW, "integer overflow");
                    return e->nr;
                }
                i = -i;
            }
            ti_val_unsafe_drop(v);
            *val = (ti_val_t *) ti_vint_create(~i);
            if (!*val)
                ex_set_mem(e);
            return e->nr;
        }
        break;
    case TI_VAL_FLOAT:
        ex_set(e, EX_TYPE_ERROR,
                "`~` not supported by type `%s`",
                ti_val_str(v));
        return e->nr;
    case TI_VAL_BOOL:
        if (preopr & PO__FLAG_BIT_INV)
        {
            _Bool b = VBOOL(v);
            if (preopr & PO__FLAG_NEGATIVE)
                b = -b;
            ti_val_unsafe_drop(v);
            *val = (ti_val_t *) ti_vint_create(~((int) b));
            return e->nr;
        }
        break;
    }

    /*
     * No ! is used in the sequence, so the number is unchanged, or the
     * negative of the current value. Boolean values must convert to either
     * 0, 1, or -1.
     */
    switch(v->tp)
    {
    case TI_VAL_INT:
        if (preopr & PO__FLAG_NEGATIVE)
        {
            int64_t i = VINT(v);
            if (i == LLONG_MIN)
            {
                ex_set(e, EX_OVERFLOW, "integer overflow");
                return e->nr;
            }
            ti_val_unsafe_drop(v);
            *val = (ti_val_t *) ti_vint_create(-i);
            if (!*val)
                ex_set_mem(e);
        }
        break;
    case TI_VAL_FLOAT:
        if (preopr & PO__FLAG_NEGATIVE)
        {
            double d = -VFLOAT(v);
            ti_val_unsafe_drop(v);
            *val = (ti_val_t *) ti_vfloat_create(d);
            if (!*val)
                ex_set_mem(e);
        }
        break;
    case TI_VAL_BOOL:
    {
        _Bool b = VBOOL(v);
        ti_val_unsafe_drop(v);
        *val = (ti_val_t *) ti_vint_create(preopr & PO__FLAG_NEGATIVE ? -b : b);
        break;
    }
    }

    return e->nr;
}
