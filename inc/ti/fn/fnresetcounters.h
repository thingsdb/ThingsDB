#include <ti/fn/fn.h>

static int do__f_reset_counters(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    if (fn_not_node_scope("reset_counters", query, e) ||
        ti_access_check_err(ti.access_node,
            query->user, TI_AUTH_CHANGE, e) ||
        fn_nargs("reset_counters", DOC_RESET_COUNTERS, 0, nargs, e))
        return e->nr;

    ti_counters_reset();

    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
