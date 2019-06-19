#include <ti/rfn/fn.h>

#define NODE_DOC_ TI_SEE_DOC("#node")

static int rq__f_node(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_node(query, nd, e));
    assert (!query->ev);    /* node queries do never create an event */
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `node` takes 0 arguments but %d %s given"NODE_DOC_,
                nargs, nargs == 1 ? "was" : "were");
        return e->nr;
    }

    query->rval = ti_node_as_qpval();
    if (!query->rval)
        ex_set_alloc(e);

    return e->nr;
}
