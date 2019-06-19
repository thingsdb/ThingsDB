#include <ti/cfn/fn.h>

#define ISFLOAT_DOC_ TI_SEE_DOC("#isfloat")

static int cq__f_isfloat(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool is_float;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `isfloat` takes 1 argument but %d were given"
                ISFLOAT_DOC_, nargs);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        return e->nr;

    is_float = ti_val_is_float(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_float);

    return e->nr;
}
