#include <ti/fn/fn.h>

static int do__f_timer_args(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_timer_t * timer;
    vec_t ** timers = ti_query_timers(query);

    if (fn_not_thingsdb_or_collection_scope("timer_args", query, e) ||
        fn_nargs("timer_args", DOC_TIMER_ARGS, 1, nargs, e))
        return e->nr;

    if (nargs == 0)
    {
        timer = ti_timer_query_alt(query, e);
    }
    else
    {
        if (ti_do_statement(query, nd->children->node, e))
            return e->nr;

        timer = ti_timer_from_val(*timers, query->rval, e);
        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }

    if (!timer)
        return e->nr;

    query->rval = (ti_val_t *) ti_tuple_from_vec(timer->args);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
