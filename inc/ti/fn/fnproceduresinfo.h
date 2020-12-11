#include <ti/fn/fn.h>

static int do__f_procedures_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    smap_t * procedures = ti_query_procedures(query);

    if (fn_not_thingsdb_or_collection_scope("procedures_info", query, e) ||
        fn_nargs("procedures_info", DOC_PROCEDURES_INFO, 0, nargs, e))
        return e->nr;

    query->rval = (ti_val_t *) ti_procedures_info(procedures, ti_access_check(
            ti_query_access(query),
            query->user,
            TI_AUTH_MODIFY));

    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
