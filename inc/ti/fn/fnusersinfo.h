#include <ti/fn/fn.h>

static int do__f_users_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_not_thingsdb_scope("users_info", query, e) ||
        /* check access */
        ti_access_check_err(
                ti.access_thingsdb,
                query->user, TI_AUTH_GRANT, e
        ) ||
        fn_nargs("users_info", DOC_USERS_INFO, 0, nargs, e))
        return e->nr;

    query->rval = (ti_val_t *) ti_users_info();
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
