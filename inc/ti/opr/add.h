#include <ti/opr/oprinc.h>

static int opr__add(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    int64_t int_ = 0;       /* set to 0 only to prevent warning */
    double float_ = 0.0;    /* set to 0 only to prevent warning */
    ti_opr_perm_t perm = TI_OPR_PERM(a, *b);
    switch(perm)
    {
    default:
        if (ti_val_is_enum(*b))
            return opr_on_enum(a, b, e, opr__add);
        ex_set(e, EX_TYPE_ERROR,
                "`+` not supported between `%s` and `%s`",
                ti_val_str(a), ti_val_str(*b));
        return e->nr;

    case OPR_INT_INT:
        if ((VINT(*b) > 0 && VINT(a) > LLONG_MAX - VINT(*b)) ||
            (VINT(*b) < 0 && VINT(a) < LLONG_MIN - VINT(*b)))
            goto overflow;
        int_ = VINT(a) + VINT(*b);
        goto type_int;

    case OPR_INT_FLOAT:
        float_ = VINT(a) + VFLOAT(*b);
        goto type_float;

    case OPR_INT_BOOL:
        if (VINT(a) == LLONG_MAX && VBOOL(*b))
            goto overflow;
        int_ = VINT(a) + VBOOL(*b);
        goto type_int;

    case OPR_FLOAT_INT:
        float_ = VFLOAT(a) + VINT(*b);
        goto type_float;

    case OPR_FLOAT_FLOAT:
        float_ = VFLOAT(a) + VFLOAT(*b);
        goto type_float;

    case OPR_FLOAT_BOOL:
        float_ = VFLOAT(a) + VBOOL(*b);
        goto type_float;

    case OPR_BOOL_INT:
        if (VBOOL(a) && VINT(*b) == LLONG_MAX)
            goto overflow;
        int_ = VBOOL(a) + VINT(*b);
        goto type_int;

    case OPR_BOOL_FLOAT:
        float_ = VBOOL(a) + VFLOAT(*b);
        goto type_float;

    case OPR_BOOL_BOOL:
        int_ = VBOOL(a) + VBOOL(*b);
        goto type_int;

    case OPR_NAME_NAME:
    case OPR_NAME_STR:
    case OPR_NAME_BYTES:
    case OPR_STR_NAME:
    case OPR_STR_STR:
    case OPR_STR_BYTES:
    case OPR_BYTES_NAME:
    case OPR_BYTES_STR:
    case OPR_BYTES_BYTES:
    {
        ti_raw_t * raw = NULL;  /* set to 0 only to prevent warning */
        raw = ti_raw_cat((ti_raw_t *) a, (ti_raw_t *) *b);
        if (raw)
        {
            ti_val_drop(*b);
            *b = (ti_val_t *) raw;
        }
        else
            ex_set_mem(e);

        return e->nr;
    }
    case OPR_ENUM_NIL ... OPR_ENUM_ERROR:
        return opr__add(VENUM(a), b, e);

    case OPR_ENUM_ENUM:
        return opr_on_enum(VENUM(a), b, e, opr__add);
    }
    assert (0);

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
}
