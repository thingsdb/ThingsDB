#include <ti/fn/fn.h>

static int do__f_timer_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_timer_t * timer;
    vec_t ** timers = ti_query_timers(query);

    if (fn_not_thingsdb_or_collection_scope("timer_info", query, e) ||
        fn_nargs("timer_info", DOC_TIMER_INFO, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    timer = ti_timer_from_val(*timers, query->rval, e);
    if (!timer)
        return e->nr;

    ti_val_unsafe_drop(query->rval);
    query->rval = ti_timer_as_mpval(timer, ti_access_check(
            ti_query_access(query),
            query->user,
            TI_AUTH_CHANGE));

    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
