#include <ti/opr/oprinc.h>

static int opr__and(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    int64_t int_ = 0;       /* set to 0 only to prevent warning */

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
            int_ = OPR__INT(a) & OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            goto type_err;
        case TI_VAL_BOOL:
            int_ = OPR__INT(a) & OPR__BOOL(*b);
            break;
        case TI_VAL_MP:
        case TI_VAL_NAME:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_WRAP:
        case TI_VAL_ARR:
        case TI_VAL_SET:
        case TI_VAL_CLOSURE:
        case TI_VAL_ERROR:
            goto type_err;
        }
        break;
    case TI_VAL_FLOAT:
        goto type_err;
    case TI_VAL_BOOL:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            int_ = OPR__BOOL(a) & OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            goto type_err;
        case TI_VAL_BOOL:
            int_ = OPR__BOOL(a) & OPR__BOOL(*b);
            break;
        case TI_VAL_MP:
        case TI_VAL_NAME:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_WRAP:
        case TI_VAL_ARR:
        case TI_VAL_SET:
        case TI_VAL_CLOSURE:
        case TI_VAL_ERROR:
            goto type_err;
        }
        break;
    case TI_VAL_MP:
    case TI_VAL_NAME:
    case TI_VAL_RAW:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ARR:
    case TI_VAL_SET:
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
        goto type_err;
    }

    if (ti_val_make_int(b, int_))
        ex_set_mem(e);

    return e->nr;

type_err:
    ex_set(e, EX_TYPE_ERROR, "bitwise `&` not supported between `%s` and `%s`",
        ti_val_str(a), ti_val_str(*b));
    return e->nr;
}
