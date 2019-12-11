#include <ti/opr/oprinc.h>

static int opr__div(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    int64_t int_ = 0;       /* set to 0 only to prevent warning */
    double float_ = 0.0;    /* set to 0 only to prevent warning */

    switch ((ti_val_enum) a->tp)
    {
    case TI_VAL_NIL:
        goto type_err;
    case TI_VAL_INT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            if (VINT(*b) == 0)
                goto zerodiv;
            if (VINT(a) == LLONG_MAX && VINT(*b) == -1)
                goto overflow;
            int_ = VINT(a) / VINT(*b);
            goto ret_int;
        case TI_VAL_FLOAT:
            if (VFLOAT(*b) == 0.0)
                goto zerodiv;
            float_ = (double) VINT(a) / VFLOAT(*b);
            goto ret_float;
        case TI_VAL_BOOL:
            if (VBOOL(*b) == 0)
                goto zerodiv;
            int_ = VINT(a) / VBOOL(*b);
            goto ret_int;
        case TI_VAL_MP:
        case TI_VAL_NAME:
        case TI_VAL_STR:
        case TI_VAL_BYTES:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_WRAP:
        case TI_VAL_ARR:
        case TI_VAL_SET:
        case TI_VAL_CLOSURE:
        case TI_VAL_ERROR:
            goto type_err;
        }
        assert (0);
        break;
    case TI_VAL_FLOAT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            if (VINT(*b) == 0)
                goto zerodiv;
            float_ = VFLOAT(a) / (double) VINT(*b);
            goto ret_float;
        case TI_VAL_FLOAT:
            if (VFLOAT(*b) == 0.0)
                goto zerodiv;
            float_ = VFLOAT(a) / VFLOAT(*b);
            goto ret_float;
        case TI_VAL_BOOL:
            if (VBOOL(*b) == 0)
                goto zerodiv;
            float_ = VFLOAT(a) / (double) VBOOL(*b);
            goto ret_float;
        case TI_VAL_MP:
        case TI_VAL_NAME:
        case TI_VAL_STR:
        case TI_VAL_BYTES:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_WRAP:
        case TI_VAL_ARR:
        case TI_VAL_SET:
        case TI_VAL_CLOSURE:
        case TI_VAL_ERROR:
            goto type_err;
        }
        assert (0);
        break;
    case TI_VAL_BOOL:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            if (VINT(*b) == 0)
                goto zerodiv;
            int_ = VBOOL(a) / VINT(*b);
            goto ret_int;
        case TI_VAL_FLOAT:
            if (VFLOAT(*b) == 0.0)
                goto zerodiv;
            float_ = (double) VBOOL(a) / VFLOAT(*b);
            goto ret_float;
        case TI_VAL_BOOL:
            if (VBOOL(*b) == 0)
                goto zerodiv;
            int_ = VBOOL(a) / VBOOL(*b);
            goto ret_int;
        case TI_VAL_MP:
        case TI_VAL_NAME:
        case TI_VAL_STR:
        case TI_VAL_BYTES:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_WRAP:
        case TI_VAL_ARR:
        case TI_VAL_SET:
        case TI_VAL_CLOSURE:
        case TI_VAL_ERROR:
            goto type_err;
        }
        assert (0);
        break;
    case TI_VAL_MP:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ARR:
    case TI_VAL_SET:
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
        goto type_err;
    }

ret_float:
    if (ti_val_make_float(b, float_))
        ex_set_mem(e);
    return e->nr;

ret_int:
    if (ti_val_make_int(b, int_))
        ex_set_mem(e);
    return e->nr;

type_err:
    ex_set(e, EX_TYPE_ERROR, "`/` not supported between `%s` and `%s`",
        ti_val_str(a), ti_val_str(*b));
    return e->nr;

overflow:
    ex_set(e, EX_OVERFLOW, "integer overflow");
    return e->nr;

zerodiv:
    ex_set(e, EX_ZERO_DIV, "division or modulo by zero");
    return e->nr;
}
