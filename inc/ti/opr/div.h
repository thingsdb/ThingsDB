#include <ti/opr/oprinc.h>

static int opr__div(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    int64_t int_ = 0;       /* set to 0 only to prevent warning */
    double float_ = 0.0;    /* set to 0 only to prevent warning */
    ti_opr_perm_t perm = TI_OPR_PERM(a, *b);
    switch(perm)
    {
    default:
        ex_set(e, EX_TYPE_ERROR,
                "`/` not supported between `%s` and `%s`",
                ti_val_str(a), ti_val_str(*b));
        return e->nr;

    case OPR_INT_INT:
        if (VINT(*b) == 0)
            goto zerodiv;
        if (VINT(a) == LLONG_MAX && VINT(*b) == -1)
            goto overflow;
        int_ = VINT(a) / VINT(*b);
        goto type_int;

    case OPR_INT_FLOAT:
        if (VFLOAT(*b) == 0.0)
            goto zerodiv;
        float_ = (double) VINT(a) / VFLOAT(*b);
        goto type_float;

    case OPR_INT_BOOL:
        if (VBOOL(*b) == 0)
            goto zerodiv;
        int_ = VINT(a) / VBOOL(*b);
        goto type_int;

    case OPR_FLOAT_INT:
        if (VINT(*b) == 0)
            goto zerodiv;
        float_ = VFLOAT(a) / (double) VINT(*b);
        goto type_float;

    case OPR_FLOAT_FLOAT:
        if (VFLOAT(*b) == 0.0)
            goto zerodiv;
        float_ = VFLOAT(a) / VFLOAT(*b);
        goto type_float;

    case OPR_FLOAT_BOOL:
        if (VBOOL(*b) == 0)
            goto zerodiv;
        float_ = VFLOAT(a) / (double) VBOOL(*b);
        goto type_float;

    case OPR_BOOL_INT:
        if (VINT(*b) == 0)
            goto zerodiv;
        int_ = VBOOL(a) / VINT(*b);
        goto type_int;

    case OPR_BOOL_FLOAT:
        if (VFLOAT(*b) == 0.0)
            goto zerodiv;
        float_ = (double) VBOOL(a) / VFLOAT(*b);
        goto type_float;

    case OPR_BOOL_BOOL:
        if (VBOOL(*b) == 0)
            goto zerodiv;
        int_ = VBOOL(a) / VBOOL(*b);
        goto type_int;
    }

type_float:
    if (ti_val_make_float(b, float_))
        ex_set_mem(e);
    return e->nr;

type_int:
    if (ti_val_make_int(b, int_))
        ex_set_mem(e);
    return e->nr;

overflow:
    ex_set(e, EX_OVERFLOW, "integer overflow");
    return e->nr;

zerodiv:
    ex_set(e, EX_ZERO_DIV, "division or modulo by zero");
    return e->nr;
}
