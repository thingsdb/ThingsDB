#include <ti/fn/fn.h>

static int do__f_nodes_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_not_node_scope("nodes_info", query, e) ||
        fn_nargs("nodes_info", DOC_NODES_INFO, 0, nargs, e))
        return e->nr;

    query->rval = ti_nodes_info_as_mpval();
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
