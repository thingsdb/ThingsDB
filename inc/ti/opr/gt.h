#include <ti/opr/oprinc.h>

static int opr__gt(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    _Bool bool_ = false;
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
            bool_ = VINT(a) > VINT(*b);
            break;
        case TI_VAL_FLOAT:
            bool_ = VINT(a) > VFLOAT(*b);
            break;
        case TI_VAL_BOOL:
            bool_ = VINT(a) > VBOOL(*b);
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
        break;
    case TI_VAL_FLOAT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            bool_ = VFLOAT(a) > VINT(*b);
            break;
        case TI_VAL_FLOAT:
            bool_ = VFLOAT(a) > VFLOAT(*b);
            break;
        case TI_VAL_BOOL:
            bool_ = VFLOAT(a) > VBOOL(*b);
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
        break;
    case TI_VAL_BOOL:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            bool_ = VBOOL(a) > VINT(*b);
            break;
        case TI_VAL_FLOAT:
            bool_ = VBOOL(a) > VFLOAT(*b);
            break;
        case TI_VAL_BOOL:
            bool_ = VBOOL(a) > VBOOL(*b);
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
        break;
    case TI_VAL_MP:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_NIL:
        case TI_VAL_INT:
        case TI_VAL_FLOAT:
        case TI_VAL_BOOL:
            goto type_err;
        case TI_VAL_MP:
        case TI_VAL_NAME:
        case TI_VAL_STR:
        case TI_VAL_BYTES:
            bool_ = ti_raw_cmp((ti_raw_t *) a, (ti_raw_t *) *b) > 0;
            break;
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
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ARR:
    case TI_VAL_SET:
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
        goto type_err;
    }

    ti_val_drop(*b);
    *b = (ti_val_t *) ti_vbool_get(bool_);

    return e->nr;

type_err:
    ex_set(e, EX_TYPE_ERROR, "`>` not supported between `%s` and `%s`",
        ti_val_str(a), ti_val_str(*b));
    return e->nr;
}
