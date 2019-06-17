#include <ti/cfn/fn.h>

static int cq__f_isinf(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool is_inf;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `isinf` takes 1 argument but %d were given", n);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
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
                "type `"TI_VAL_FLOAT_S"` but got type `%s` instead",
                ti_val_str(query->rval));
        return e->nr;
    }

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_inf);

    return e->nr;
}
