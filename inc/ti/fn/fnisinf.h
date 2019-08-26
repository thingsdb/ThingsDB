#include <ti/fn/fn.h>

#define ISINF_DOC_ TI_SEE_DOC("#isinf")

static int do__f_isinf(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    _Bool is_inf;

    if (fn_chained("isinf", query, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `isinf` takes 1 argument but %d were given"
                ISINF_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_scope(query, nd->children->node, e))
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
        ex_set(e, EX_BAD_DATA,
                "function `isinf` expects argument 1 to be of "
                "type `"TI_VAL_FLOAT_S"` but got type `%s` instead"ISINF_DOC_,
                ti_val_str(query->rval));
        return e->nr;
    }

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_inf);

    return e->nr;
}
