#include <ti/opr/oprinc.h>

static int opr__mul(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    int64_t int_ = 0;       /* set to 0 only to prevent warning */
    double float_ = 0.0f;   /* set to 0 only to prevent warning */

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
            if ((OPR__INT(a) > LLONG_MAX / OPR__INT(*b)) ||
                (OPR__INT(a) < LLONG_MIN / OPR__INT(*b)) ||
                (OPR__INT(a) == -1 && OPR__INT(*b) == LLONG_MIN) ||
                (OPR__INT(*b) == -1 && OPR__INT(a) == LLONG_MIN))
                goto overflow;
            int_ = OPR__INT(a) * OPR__INT(*b);
            goto type_int;
        case TI_VAL_FLOAT:
            float_ = OPR__INT(a) * OPR__FLOAT(*b);
            goto type_float;
        case TI_VAL_BOOL:
            int_ = OPR__INT(a) * OPR__BOOL(*b);
            goto type_int;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_SET:
        case TI_VAL_CLOSURE:
        case TI_VAL_ERROR:
            goto type_err;
        }
        break;
    case TI_VAL_FLOAT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            float_ = OPR__FLOAT(a) * OPR__INT(*b);
            goto type_float;
        case TI_VAL_FLOAT:
            float_ = OPR__FLOAT(a) * OPR__FLOAT(*b);
            goto type_float;
        case TI_VAL_BOOL:
            float_ = OPR__FLOAT(a) * OPR__BOOL(*b);
            goto type_float;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_SET:
        case TI_VAL_CLOSURE:
        case TI_VAL_ERROR:
            goto type_err;
        }
        break;
    case TI_VAL_BOOL:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            int_ = OPR__BOOL(a) * OPR__INT(*b);
            goto type_int;
        case TI_VAL_FLOAT:
            float_ = OPR__BOOL(a) * OPR__FLOAT(*b);
            goto type_float;
        case TI_VAL_BOOL:
            int_ = OPR__BOOL(a) * OPR__BOOL(*b);
            goto type_int;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_SET:
        case TI_VAL_CLOSURE:
        case TI_VAL_ERROR:
            goto type_err;
        }
        break;
    case TI_VAL_QP:
    case TI_VAL_RAW:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_ARR:
    case TI_VAL_SET:
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
        goto type_err;
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

type_err:
    ex_set(e, EX_BAD_DATA, "`*` not supported between `%s` and `%s`",
        ti_val_str(a), ti_val_str(*b));
    return e->nr;

overflow:
    ex_set(e, EX_OVERFLOW, "integer overflow");
    return e->nr;
}
