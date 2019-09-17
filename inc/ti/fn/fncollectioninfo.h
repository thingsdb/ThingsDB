#include <ti/fn/fn.h>

#define COLLECTION_INFO_DOC_ TI_SEE_DOC("#collection_info")

static int do__f_collection_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    ti_collection_t * collection;

    if (fn_not_thingsdb_scope("collection_info", query, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_NUM_ARGUMENTS,
                "function `collection_info` takes 1 argument but %d were given"
                COLLECTION_INFO_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    collection = ti_collections_get_by_val(query->rval, e);
    if (e->nr)
        return e->nr;

    assert (collection);


    ti_val_drop(query->rval);
    query->rval = ti_collection_as_qpval(collection);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
