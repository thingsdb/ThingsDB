#include <ti/fn/fn.h>

static int do__f_del_timer(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_timer_t * timer;
    ti_task_t * task;

    if (fn_not_thingsdb_or_collection_scope("del_timer", query, e) ||
        fn_nargs_max("del_timer", DOC_DEL_TIMER, 1, nargs, e))
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
                    "function `del_timer` requires at least 1 argument "
                    "when used outside a timer but 0 were given"
                    DOC_DEL_TIMER);
            return e->nr;
        }
        timer = query->with.timer;
    }

    query->rval = (ti_val_t *) ti_nil_get();
    ti_timer_mark_del(timer);

    task = ti_task_get_task(
            query->change,
            query->collection ? query->collection->root : ti.thing0);

    if (!task || ti_task_add_del_timer(task, timer))
        ex_set_mem(e);  /* task cleanup is not required */

    return e->nr;
}
