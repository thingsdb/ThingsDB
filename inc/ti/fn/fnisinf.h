#include <ti/fn/fn.h>

static int do__f_is_inf(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    _Bool is_inf;

    if (fn_nargs("is_inf", DOC_IS_INF, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    switch (query->rval->tp)
    {
    case TI_VAL_BOOL:
    case TI_VAL_INT:
        is_inf = false;
        break;
    case TI_VAL_FLOAT:
        is_inf = isinf(VFLOAT(query->rval));
        break;
    default:
        ex_set(e, EX_TYPE_ERROR,
                "function `is_inf` expects argument 1 to be of "
                "type `"TI_VAL_FLOAT_S"` but got type `%s` instead"DOC_IS_INF,
                ti_val_str(query->rval));
        return e->nr;
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_inf);

    return e->nr;
}
