#include <ti/fn/fn.h>

static int do__f_time_zones_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    if (fn_not_thingsdb_scope("time_zones_info", query, e) ||
        fn_nargs("time_zones_info", DOC_TIME_ZONES_INFO, 0, nargs, e))
        return e->nr;

    query->rval = ti_tz_as_mpval();
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
