#include <ti/fn/fn.h>

static int do__f_collection_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_collection_t * collection;
    ti_user_t * user = query->user;

    if (fn_not_thingsdb_scope("collection_info", query, e) ||
        fn_nargs("collection_info", DOC_COLLECTION_INFO, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (ti_access_check(ti.access_thingsdb, user, TI_AUTH_EVENT))
    {
        /*
         * Only if the user has no `EVENT` permissions in the thingsdb scope,
         * then only collection info for collections where the user has at
         * least read permissions are returned.
         */
        user = NULL;
    }

    collection = ti_collections_get_by_val(query->rval, e);
    if (e->nr)
        return e->nr;

    if (user && ti_access_check_or_err(
            collection->access,
            user,
            TI_AUTH_QUERY|TI_AUTH_RUN,
            e))
        return e->nr;

    assert (collection);

    ti_val_unsafe_drop(query->rval);
    query->rval = ti_collection_as_mpval(collection);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
