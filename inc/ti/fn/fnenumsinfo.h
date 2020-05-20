#include <ti/fn/fn.h>

static int do__f_enums_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_not_collection_scope("enums_info", query, e) ||
        fn_nargs("enums_info", DOC_ENUMS_INFO, 0, nargs, e))
        return e->nr;

    query->rval = (ti_val_t *) ti_enums_info(query->collection->enums);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
