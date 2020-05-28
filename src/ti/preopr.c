/*
 * ti/preopr.c
 */
#include <assert.h>
#include <ti/preopr.h>
#include <ti/vint.h>
#include <ti/vfloat.h>
#include <ti/vbool.h>
#include <util/logger.h>

enum
{
    PO__FALSE,
    PO__BOOL,
    PO__NEGATIVE,
    PO__AS_NUM,
    PO__CHK_NUM,
};

enum
{
    PO__FLAG_FALSE    =1<<PO__FALSE,
    PO__FLAG_BOOL     =1<<PO__BOOL,
    PO__FLAG_NEGATIVE =1<<PO__NEGATIVE,
    PO__FLAG_AS_NUM   =1<<PO__AS_NUM,
    PO__FLAG_CHK_NUM  =1<<PO__CHK_NUM,
};

int ti_preopr_bind(const char * s, size_t n)
{
    register int preopr = 0;
    register int negative = 0;
    register int nots = 0;

    if (!n)
        return preopr;

    switch(*s)
    {
    case '-':
        ++negative;
        /* fall through */
    case '+':
        preopr |= 1 << PO__AS_NUM;
        break;
    case '!':
        ++nots;
    }

    while (--n)
    {
        switch(*(++s))
        {
        case '-':
            negative += !nots;
            break;
        case '!':
            ++nots;
        }
    }
    preopr |= (
        ((nots & 1) << PO__FALSE) |
        ((!!nots) << PO__BOOL) |
        ((negative & 1) << PO__NEGATIVE) |
        ((*s != '!') << PO__CHK_NUM)
    );

    return preopr;
}

int ti_preopr_calc(int preopr, ti_val_t ** val, ex_t * e)
{
    ti_val_t * v = *val;

    if ((preopr & PO__FLAG_CHK_NUM) &&
        v->tp != TI_VAL_INT &&
        v->tp != TI_VAL_FLOAT &&
        v->tp != TI_VAL_BOOL)
    {
        ex_set(e, EX_TYPE_ERROR,
                "`-/+` not supported by type `%s`",
                ti_val_str(v));
        return e->nr;
    }

    if (preopr & PO__FLAG_BOOL)
    {
        _Bool b = (preopr & PO__FLAG_FALSE) ^ ti_val_as_bool(v);

        ti_val_drop(v);

        *val = preopr & PO__FLAG_AS_NUM
            ? (ti_val_t *) ti_vint_create(preopr & PO__FLAG_NEGATIVE ? -b : b)
            : (ti_val_t *) ti_vbool_get(b);

        return 0;
    }

    assert (preopr & PO__FLAG_AS_NUM);

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
            ti_val_drop(v);
            *val = (ti_val_t *) ti_vint_create(-i);
            if (!*val)
                ex_set_mem(e);
        }
        break;
    case TI_VAL_FLOAT:
        if (preopr & PO__FLAG_NEGATIVE)
        {
            double nd = -VFLOAT(v);
            ti_val_drop(v);
            *val = (ti_val_t *) ti_vfloat_create(nd);
            if (!*val)
                ex_set_mem(e);
        }
        break;
    case TI_VAL_BOOL:
    {
        _Bool b = VBOOL(v);

        ti_val_drop(v);
        *val = (ti_val_t *) ti_vint_create(preopr & PO__FLAG_NEGATIVE ? -b : b);
        break;
    }
    }

    return e->nr;
}
