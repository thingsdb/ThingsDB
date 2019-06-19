#include <ti/cfn/fn.h>

#define ISLIST_DOC_ TI_SEE_DOC("#islist")

static int cq__f_islist(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool is_list;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `islist` takes 1 argument but %d were given"
                ISLIST_DOC_, nargs);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        return e->nr;

    is_list = ti_val_is_list(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_list);

    return e->nr;
}
