#include <ti/fn/fn.h>

static int do__f_node_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    if (fn_not_node_scope("node_info", query, e) ||
        fn_nargs("node_info", DOC_NODE_INFO, 0, nargs, e))
        return e->nr;

    query->rval = ti_this_node_as_mpval();
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
