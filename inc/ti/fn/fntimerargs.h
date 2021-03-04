#include <ti/fn/fn.h>

static int do__f_timer_args(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_timer_t * timer;
    vec_t * args;
    vec_t ** timers = ti_query_timers(query);
    size_t i, m, n;

    if (fn_not_thingsdb_or_collection_scope("timer_args", query, e) ||
        fn_nargs("timer_args", DOC_TIMER_ARGS, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    timer = ti_timer_from_val(*timers, query->rval, e);
    if (!timer)
        return e->nr;

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    n = timer->args->n ? timer->args->n-1 : 0;

    args = vec_new(n);
    if (!args)
    {
        ex_set_mem(e);
        return e->nr;
    }

    for (i=1, m=timer->args->n; i < m; ++i)
    {
        ti_val_t * v = VEC_get(timer->args, i);
        VEC_push(args, v);
        ti_incref(v);
    }

    query->rval = (ti_val_t *) ti_tuple_from_vec(args);
    if (!query->rval)
    {
        ex_set_mem(e);
        vec_destroy(args, (vec_destroy_cb) ti_val_unsafe_drop);
    }

    return e->nr;
}
