#include <ti/fn/fn.h>

static int do__f_timers_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    _Bool with_definition;
    vec_t ** timers = ti_query_timers(query);

    if (fn_not_thingsdb_or_collection_scope("timers_info", query, e) ||
        fn_nargs("timers_info", DOC_TIMERS_INFO, 0, nargs, e))
        return e->nr;

    with_definition = ti_access_check(
            ti_query_access(query),
            query->user,
            TI_AUTH_CHANGE);

    query->rval = (ti_val_t *) ti_timers_info(*timers, with_definition);

    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
