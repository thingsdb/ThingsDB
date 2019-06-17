#include <ti/cfn/fn.h>

static int cq__f_isarray(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool is_array;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `isarray` takes 1 argument but %d were given", n);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        return e->nr;

    is_array = ti_val_is_array(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_array);

    return e->nr;
}
