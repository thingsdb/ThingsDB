#include <ti/fn/fn.h>

static int do__f_set_timer_args(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_timer_t * timer;
    ti_task_t * task;
    size_t m, n, idx = 0;

    if (fn_not_thingsdb_or_collection_scope("set_timer_args", query, e) ||
        fn_nargs_range("set_timer_args", DOC_SET_TIMER_ARGS, 1, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (nargs == 2)
    {
        vec_t ** timers = ti_query_timers(query);

        timer = ti_timer_from_val(*timers, query->rval, e);
        if (!timer)
            return e->nr;   /* e must be set */

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;

        if (ti_do_statement(query, nd->children->next->next->node, e) ||
            fn_arg_array(
                    "set_timer_args", DOC_SET_TIMER_ARGS, 2, query->rval, e))
            return e->nr;
    }
    else
    {
        if (query->with_tp != TI_QUERY_WITH_TIMER)
        {
            ex_set(e, EX_NUM_ARGUMENTS,
                    "function `set_timer_args` requires at least 2 arguments "
                    "when used outside a timer but 1 was given"
                    DOC_SET_TIMER_ARGS);
            return e->nr;
        }
        timer = query->with.timer;

        if (!ti_val_is_array(query->rval))
        {
            ex_set(e, EX_NUM_ARGUMENTS,
                    "function `set_timer_args` expects a "
                    "`"TI_VAL_LIST_S"` or `"TI_VAL_TUPLE_S"` when used "
                    "with a single argument but got type `%s` instead"
                    DOC_SET_TIMER_ARGS);
            return e->nr;
        }
    }

    m = timer->args->n;

    if (!query->collection &&
        ti_timer_check_thingsdb_args(VARR(query->rval), e))
        return e->nr;

    n = VARR(query->rval)->n;
    if (n > m)
    {
        ex_set(e, EX_NUM_ARGUMENTS,
                "got %zu timer argument%s while the given closure "
                "accepts no more than %zu argument%s"DOC_SET_TIMER_ARGS,
                n, n == 1 ? "" : "s",
                m, m == 1 ? "" : "s");
        return e->nr;
    }

    for (vec_each(VARR(query->rval), ti_val_t, v), ++idx)
    {
        /*
         * No garbage collection safe drop is required as all the possible
         * things are guaranteed to have Id's.
         */
        ti_val_unsafe_drop(vec_set(timer->args, v, idx));
        ti_incref(v);
    }

    while (idx < m)
        ti_val_unsafe_drop(vec_set(timer->args, ti_nil_get(), idx++));

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    task = ti_task_get_task(
            query->change,
            query->collection ? query->collection->root : ti.thing0);

    if (!task || ti_task_add_set_timer_args(task, timer))
        ex_set_mem(e);  /* task cleanup is not required */

    return e->nr;
}
