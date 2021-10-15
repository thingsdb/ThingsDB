#include <ti/fn/fn.h>

static int do__f_timer_args(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_timer_t * timer;
    vec_t * args;
    size_t i, m, n;

    if (fn_not_thingsdb_or_collection_scope("timer_args", query, e) ||
        fn_nargs_max("timer_args", DOC_TIMER_ARGS, 1, nargs, e))
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
        query->rval = NULL;
    }
    else
    {
        if (query->with_tp != TI_QUERY_WITH_TIMER)
        {
            ex_set(e, EX_NUM_ARGUMENTS,
                    "function `timer_args` requires at least 1 argument "
                    "when used outside a timer but 0 were given"
                    DOC_TIMER_ARGS);
            return e->nr;
        }
        timer = query->with.timer;
    }

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
