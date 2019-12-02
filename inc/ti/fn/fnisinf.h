#include <ti/fn/fn.h>

static int do__f_isinf(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool is_inf;

    if (fn_nargs("isinf", DOC_ISINF, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    switch (query->rval->tp)
    {
    case TI_VAL_BOOL:
    case TI_VAL_INT:
        is_inf = false;
        break;
    case TI_VAL_FLOAT:
        is_inf = isinf(((ti_vfloat_t *) query->rval)->float_);
        break;
    default:
        ex_set(e, EX_TYPE_ERROR,
                "function `isinf` expects argument 1 to be of "
                "type `"TI_VAL_FLOAT_S"` but got type `%s` instead"DOC_ISINF,
                ti_val_str(query->rval));
        return e->nr;
    }

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_inf);

    return e->nr;
}
