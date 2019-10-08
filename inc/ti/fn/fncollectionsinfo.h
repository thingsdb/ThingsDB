#include <ti/fn/fn.h>

static int do__f_collections_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_not_thingsdb_scope("collections_info", query, e) ||
        fn_nargs("collections_info", DOC_COLLECTIONS_INFO, 0, nargs, e))
        return e->nr;

    query->rval = ti_collections_as_mpval();
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
