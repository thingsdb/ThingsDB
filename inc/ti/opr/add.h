#include <ti/opr/oprinc.h>

static int opr__add(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    int64_t int_ = 0;       /* set to 0 only to prevent warning */
    double float_ = 0.0;    /* set to 0 only to prevent warning */
    ti_raw_t * raw = NULL;  /* set to 0 only to prevent warning */

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
            if ((VINT(*b) > 0 && VINT(a) > LLONG_MAX - VINT(*b)) ||
                (VINT(*b) < 0 && VINT(a) < LLONG_MIN - VINT(*b)))
                goto overflow;
            int_ = VINT(a) + VINT(*b);
            goto type_int;
        case TI_VAL_FLOAT:
            float_ = VINT(a) + VFLOAT(*b);
            goto type_float;
        case TI_VAL_BOOL:
            if (VINT(a) == LLONG_MAX && VBOOL(*b))
                goto overflow;
            int_ = VINT(a) + VBOOL(*b);
            goto type_int;
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
            float_ = VFLOAT(a) + VINT(*b);
            goto type_float;
        case TI_VAL_FLOAT:
            float_ = VFLOAT(a) + VFLOAT(*b);
            goto type_float;
        case TI_VAL_BOOL:
            float_ = VFLOAT(a) + VBOOL(*b);
            goto type_float;
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
            if (VBOOL(a) && VINT(*b) == LLONG_MAX)
                goto overflow;
            int_ = VBOOL(a) + VINT(*b);
            goto type_int;
        case TI_VAL_FLOAT:
            float_ = VBOOL(a) + VFLOAT(*b);
            goto type_float;
        case TI_VAL_BOOL:
            int_ = VBOOL(a) + VBOOL(*b);
            goto type_int;
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
        goto type_err;
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_NIL:
        case TI_VAL_INT:
        case TI_VAL_FLOAT:
        case TI_VAL_BOOL:
        case TI_VAL_MP:
            goto type_err;
        case TI_VAL_NAME:
        case TI_VAL_STR:
        case TI_VAL_BYTES:
            raw = ti_raw_cat((ti_raw_t *) a, (ti_raw_t *) *b);
            goto type_raw;
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

    assert (0);

type_raw:
    if (raw)
    {
        ti_val_drop(*b);
        *b = (ti_val_t *) raw;
    }
    else
        ex_set_mem(e);

    return e->nr;

type_float:

    if (ti_val_make_float(b, float_))
        ex_set_mem(e);

    return e->nr;

type_int:

    if (ti_val_make_int(b, int_))
        ex_set_mem(e);

    return e->nr;

type_err:
    ex_set(e, EX_TYPE_ERROR, "`+` not supported between `%s` and `%s`",
        ti_val_str(a), ti_val_str(*b));
    return e->nr;

overflow:
    ex_set(e, EX_OVERFLOW, "integer overflow");
    return e->nr;
}
