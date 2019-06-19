#include <ti/cfn/fn.h>

#define RET_DOC_ TI_SEE_DOC("#ret")

static int cq__f_ret(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `ret` takes 0 arguments but %d %s given"RET_DOC_,
                nargs, nargs == 1 ? "was" : "were");
        return e->nr;
    }

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
