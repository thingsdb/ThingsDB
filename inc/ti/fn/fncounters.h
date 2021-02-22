#include <ti/fn/fn.h>

static int do__f_counters(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    if (fn_not_node_scope("counters", query, e) ||
        fn_nargs("counters", DOC_COUNTERS, 0, nargs, e))
        return e->nr;

    query->rval = ti_counters_as_mpval();
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
