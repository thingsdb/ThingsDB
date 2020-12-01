#include <ti/opr/oprinc.h>

static int opr__le(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    _Bool bool_ = false;
    ti_opr_perm_t perm = TI_OPR_PERM(a, *b);
    switch(perm)
    {
    default:
        ex_set(e, EX_TYPE_ERROR,
                "`<=` not supported between `%s` and `%s`",
                ti_val_str(a), ti_val_str(*b));
        return e->nr;

    case OPR_INT_INT:
        bool_ = VINT(a) <= VINT(*b);
        break;

    case OPR_INT_FLOAT:
        bool_ = VINT(a) <= VFLOAT(*b);
        break;

    case OPR_INT_BOOL:
        bool_ = VINT(a) <= VBOOL(*b);
        break;

    case OPR_FLOAT_INT:
        bool_ = VFLOAT(a) <= VINT(*b);
        break;

    case OPR_FLOAT_FLOAT:
        bool_ = VFLOAT(a) <= VFLOAT(*b);
        break;

    case OPR_FLOAT_BOOL:
        bool_ = VFLOAT(a) <= VBOOL(*b);
        break;

    case OPR_BOOL_INT:
        bool_ = VBOOL(a) <= VINT(*b);
        break;

    case OPR_BOOL_FLOAT:
        bool_ = VBOOL(a) <= VFLOAT(*b);
        break;

    case OPR_BOOL_BOOL:
        bool_ = VBOOL(a) <= VBOOL(*b);
        break;

    case OPR_DATETIME_DATETIME:
        bool_ = DATETIME(a) <= DATETIME(*b);
        break;

    case OPR_NAME_NAME:
    case OPR_NAME_STR:
    case OPR_NAME_BYTES:
    case OPR_STR_DATETIME:
    case OPR_STR_NAME:
    case OPR_STR_STR:
    case OPR_STR_BYTES:
    case OPR_BYTES_NAME:
    case OPR_BYTES_STR:
    case OPR_BYTES_BYTES:
        bool_ = ti_raw_cmp((ti_raw_t *) a, (ti_raw_t *) *b) <= 0;
        break;
    }

    ti_val_unsafe_drop(*b);
    *b = (ti_val_t *) ti_vbool_get(bool_);
    return e->nr;
}
