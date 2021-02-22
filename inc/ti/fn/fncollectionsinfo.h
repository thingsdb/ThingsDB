#include <ti/fn/fn.h>

static int do__f_collections_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_user_t * user = query->user;

    if (fn_not_thingsdb_scope("collections_info", query, e) ||
        fn_nargs("collections_info", DOC_COLLECTIONS_INFO, 0, nargs, e))
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

    query->rval = (ti_val_t *) ti_collections_info(user);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
