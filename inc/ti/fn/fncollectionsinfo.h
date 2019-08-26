#include <ti/fn/fn.h>

#define COLLECTIONS_INFO_DOC_ TI_SEE_DOC("#collections_info")

static int do__f_collections_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    if (fn_not_thingsdb_scope("collections_info", query, e))
        return e->nr;

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `collections_info` takes 0 arguments but %d %s given"
                COLLECTIONS_INFO_DOC_, nargs, nargs == 1 ? "was" : "were");
        return e->nr;
    }

    query->rval = ti_collections_as_qpval();
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
