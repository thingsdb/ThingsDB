#include <ti/fn/fn.h>

static int do__f_timer_at(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_timer_t * timer;

    if (fn_not_thingsdb_or_collection_scope("timer_at", query, e) ||
        fn_nargs_max("timer_at", DOC_TIMER_AT, 1, nargs, e))
        return e->nr;

    if (nargs == 1)
    {
        vec_t ** timers = ti_query_timers(query);

        if (ti_do_statement(query, nd->children->node, e))
            return e->nr;

        timer = ti_timer_from_val(*timers, query->rval, e);
        if (!timer)
            return e->nr;   /* e must be set */

        ti_val_unsafe_drop(query->rval);
    }
    else
    {
        if (query->with_tp != TI_QUERY_WITH_TIMER)
        {
            ex_set(e, EX_NUM_ARGUMENTS,
                    "function `timer_at` requires at least 1 argument "
                    "when used outside a timer but 0 were given"
                    DOC_TIMER_AT);
            return e->nr;
        }
        timer = query->with.timer;
    }

    query->rval = (ti_val_t *) ti_datetime_from_u64(
            timer->next_run,
            query->collection->tz);

    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
