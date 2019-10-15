#include <ti/fn/fn.h>

static int do__f_collection_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_collection_t * collection;

    if (fn_not_thingsdb_scope("collection_info", query, e) ||
        fn_nargs("collection_info", DOC_COLLECTION_INFO, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    collection = ti_collections_get_by_val(query->rval, e);
    if (e->nr)
        return e->nr;

    assert (collection);

    ti_val_drop(query->rval);
    query->rval = ti_collection_as_mpval(collection);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
