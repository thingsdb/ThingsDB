#include <ti/cfn/fn.h>

#define ISBOOL_DOC_ TI_SEE_DOC("#isbool")

static int cq__f_isbool(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool is_bool;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `isbool` takes 1 argument but %d were given"
                ISBOOL_DOC_, nargs);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        return e->nr;

    is_bool = ti_val_is_bool(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_bool);

    return e->nr;
}
