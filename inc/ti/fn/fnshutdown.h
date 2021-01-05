#include <ti/fn/fn.h>

static int do__f_shutdown(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_not_node_scope("shutdown", query, e) ||
        ti_access_check_err(ti.access_node, query->user, TI_AUTH_EVENT, e) ||
        fn_nargs("shutdown", DOC_SHUTDOWN, 0, nargs, e))
        return e->nr;

    ti_term(SIGINT);

    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
