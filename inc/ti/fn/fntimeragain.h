#include <ti/fn/fn.h>

static int do__f_timer_again(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_task_t * task;
    ti_timer_t * timer;
    time_t again_ts;

    if (fn_nargs("timer_again", DOC_TIMER_AGAIN, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_datetime("timer_again", DOC_TIMER_AGAIN, 1, query->rval, e))
        return e->nr;

    if (query->with_tp != TI_QUERY_WITH_TIMER)
    {
        ex_set(e, EX_OPERATION,
                "function `timer_again` cannot be used outside a timer"
                DOC_TIMER_AGAIN);
        return e->nr;
    }

    timer = query->with.timer;
    if (timer->repeat)
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `timer_again` cannot be used with repeating timers"
                DOC_TIMER_AGAIN);
        return e->nr;
    }

    again_ts = DATETIME(query->rval);
    if ((uint64_t) again_ts <= timer->next_run + TI_TIMERS_MIN_REPEAT)
    {
        ex_set(e, EX_VALUE_ERROR,
                "new start time must be at least one minute (%d seconds) "
                "after the previous start time"
                DOC_TIMER_AGAIN,
                TI_TIMERS_MIN_REPEAT);
        return e->nr;
    }
    if (again_ts > UINT32_MAX)
    {
        ex_set(e, EX_VALUE_ERROR, "start time out-of-range"DOC_TIMER_AGAIN);
        return e->nr;
    }

    timer->next_run = (uint64_t) again_ts;

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    task = ti_task_get_task(
            query->change,
            query->collection ? query->collection->root : ti.thing0);

    if (task)
        (void) ti_task_add_timer_again(task, timer);

    return e->nr;
}
