#include <ti/opr/oprinc.h>

static int opr__mod(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    int64_t int_ = 0;       /* set to 0 only to prevent warning */
    ti_opr_perm_t perm = TI_OPR_PERM(a, *b);

    switch (perm)
    {
    default:
        if (ti_val_is_enum(*b))
            return opr_on_enum(a, b, e, opr__mod);

        ex_set(e, EX_TYPE_ERROR,
                "`%` not supported between `%s` and `%s`",
                ti_val_str(a), ti_val_str(*b));
        return e->nr;

    case OPR_INT_INT:
        if (VINT(*b) == 0)
            goto zerodiv;
        int_ = VINT(a) % VINT(*b);
        break;

    case OPR_INT_FLOAT:
        if (ti_val_overflow_cast(VFLOAT(*b)))
            goto overflow;
        int_ = (int64_t) VFLOAT(*b);
        if (int_ == 0)
            goto zerodiv;
        int_ = VINT(a) % int_;
        break;

    case OPR_INT_BOOL:
        if (VBOOL(*b) == 0)
            goto zerodiv;
        int_ = VINT(a) % VBOOL(*b);
        break;

    case OPR_FLOAT_INT:
        if (ti_val_overflow_cast(VFLOAT(a)))
            goto overflow;
        if (VINT(*b) == 0)
            goto zerodiv;
        int_ = (int64_t) VFLOAT(a) % VINT(*b);
        break;

    case OPR_FLOAT_FLOAT:
        if (ti_val_overflow_cast(VFLOAT(a)) ||
            ti_val_overflow_cast(VFLOAT(*b)))
            goto overflow;
        int_ = (int64_t) VFLOAT(*b);
        if (int_ == 0)
            goto zerodiv;
        int_ = (int64_t) VFLOAT(a) % int_;
        break;

    case OPR_FLOAT_BOOL:
        if (ti_val_overflow_cast(VFLOAT(a)))
            goto overflow;
        if (VBOOL(*b) == 0)
            goto zerodiv;
        int_ = (int64_t) VFLOAT(a) % VBOOL(*b);
        break;

    case OPR_BOOL_INT:
        if (VINT(*b) == 0)
            goto zerodiv;
        int_ = VBOOL(a) % VINT(*b);
        break;

    case OPR_BOOL_FLOAT:
        if (ti_val_overflow_cast(VFLOAT(*b)))
            goto overflow;
        int_ = (int64_t) VFLOAT(*b);
        if (int_ == 0)
            goto zerodiv;
        int_ = VBOOL(a) % int_;
        break;

    case OPR_BOOL_BOOL:
        if (VBOOL(*b) == 0)
            goto zerodiv;
        int_ = VBOOL(a) % VBOOL(*b);
        break;

    case OPR_ENUM_NIL ... OPR_ENUM_ERROR:
        return opr__mod(VENUM(a), b, e);

    case OPR_ENUM_ENUM:
        return opr_on_enum(VENUM(a), b, e, opr__mod);
    }

    if (ti_val_make_int(b, int_))
        ex_set_mem(e);
    return e->nr;  /* success */

overflow:
    ex_set(e, EX_OVERFLOW, "integer overflow");
    return e->nr;

zerodiv:
    ex_set(e, EX_ZERO_DIV, "division or modulo by zero");
    return e->nr;
}
