#include <ti/fn/fn.h>

static int do__f_has_timer(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    _Bool has_timer;
    ti_timer_t * timer;
    vec_t ** timers = ti_query_timers(query);

    if (fn_not_thingsdb_or_collection_scope("has_timer", query, e) ||
        fn_nargs("has_timer", DOC_HAS_TIMER, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_int("has_timer", DOC_HAS_TIMER, 1, query->rval, e))
        return e->nr;

    timer = ti_timer_by_id(*timers, (uint64_t) VINT(query->rval));
    has_timer = timer && timer->user;

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(has_timer);

    return e->nr;
}
