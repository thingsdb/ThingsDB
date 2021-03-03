#include <ti/fn/fn.h>

static int do__f_set_timer_args(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_timer_t * timer;
    ti_task_t * task;
    size_t i = 1, m;
    vec_t ** timers = ti_query_timers(query);

    if (fn_not_thingsdb_or_collection_scope("set_timer_args", query, e) ||
        fn_nargs("set_timer_args", DOC_SET_TIMER_ARGS, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    timer = ti_timer_from_val(*timers, query->rval, e);
    if (!timer)
        return e->nr;   /* e must be set */

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next->node, e) ||
        fn_arg_array("set_timer_args", DOC_SET_TIMER_ARGS, 2, query->rval, e))
        return e->nr;

    m = timer->args->n;

    if (!query->collection &&
        ti_timer_check_thingsdb_args(VARR(query->rval), e))
        return e->nr;

    for (vec_each(VARR(query->rval), ti_val_t, v), ++i)
    {
        if (i >= m)
            break;

        ti_val_unsafe_gc_drop(vec_set(timer->args, v, i));
        ti_incref(v);
    }

    while (i < m)
        ti_val_unsafe_gc_drop(vec_set(timer->args, ti_nil_get(), i++));

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    task = ti_task_get_task(
            query->ev,
            query->collection ? query->collection->root : ti.thing0);

    if (!task || ti_task_add_set_timer_args(task, timer))
        ex_set_mem(e);  /* task cleanup is not required */

    return e->nr;
}
