#include <ti/fn/fn.h>

static int do__f_new_timer(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    cleri_children_t * child;
    ti_task_t * task;
    ti_timer_t * timer;
    ti_closure_t * closure;
    time_t start_ts;
    vec_t * args;
    uint32_t repeat = 0;
    uint64_t scope_id = ti_query_scope_id(query);
    vec_t ** timers = ti_query_timers(query);
    ti_vint_t * timer_id;
    size_t m, n;

    if (fn_not_thingsdb_or_collection_scope("new_timer", query, e) ||
        fn_nargs_range("new_timer", DOC_NEW_TIMER, 2, 4, nargs, e) ||
        ti_do_statement(query, (child = nd->children)->node, e) ||
        fn_arg_datetime("new_timer", DOC_NEW_TIMER, 1, query->rval, e))
        return e->nr;

    start_ts = DATETIME(query->rval);
    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if ((*timers)->n >= TI_MAX_TIMER_COUNT)
    {
        ex_set(e, EX_MAX_QUOTA,
                "maximum number of timers (%u) is reached",
                TI_MAX_TIMER_COUNT);
        return e->nr;
    }

    if (start_ts <= 0 || start_ts > UINT32_MAX)
    {
        ex_set(e, EX_VALUE_ERROR, "start time out-of-range"DOC_NEW_TIMER);
        return e->nr;
    }

    if (ti_do_statement(query, (child = child->next->next)->node, e))
        return e->nr;

    if (ti_val_is_int(query->rval))
    {
        if (VINT(query->rval) < TI_TIMERS_MIN_REPEAT)
        {
            ex_set(e, EX_VALUE_ERROR,
                "repeat value must be at least one "
                "minute (%d seconds) or disabled (nil)"
                DOC_NEW_TIMER,
                TI_TIMERS_MIN_REPEAT);
            return e->nr;
        }
        if (VINT(query->rval) > UINT32_MAX)
        {
            ex_set(e, EX_VALUE_ERROR,
                "repeat value out-of-range"DOC_NEW_TIMER);
            return e->nr;
        }
        repeat = (uint32_t) VINT(query->rval);
    }
    else if (!ti_val_is_nil(query->rval))
        goto skip_repeat;

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    child = child->next ? child->next->next : NULL;

    if (!child)
    {
        ex_set(e, EX_NUM_ARGUMENTS,
            "function `new_timer` is missing the `callback` argument"
            DOC_NEW_TIMER);
        return e->nr;
    }

    if (ti_do_statement(query, child->node, e))
        return e->nr;

skip_repeat:

    if (!ti_val_is_closure(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `new_timer` requires a `"TI_VAL_CLOSURE_S"` "
                "as `callback` argument but got type `%s` instead"DOC_NEW_TIMER,
                ti_val_str(query->rval));
        return e->nr;
    }

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (ti_closure_unbound(closure, e))
        goto fail1;

    m = closure->vars->n;

    if (!(args = vec_new(m)) || vec_reserve(timers, 1))
    {
        ex_set_mem(e);
        goto fail2;
    }

    child = child->next ? child->next->next : NULL;

    if (child)
    {
        if (ti_do_statement(query, child->node, e))
            goto fail2;

        child = child->next ? child->next->next : NULL;

        if (child)
        {
            ex_set(e, EX_NUM_ARGUMENTS,
                "function `new_timer` got too many arguments"DOC_NEW_TIMER);
            goto fail2;
        }

        if (!ti_val_is_array(query->rval))
        {
            ex_set(e, EX_TYPE_ERROR,
                    "function `new_timer` expects callback arguments to be of "
                    "type `"TI_VAL_LIST_S"` or `"TI_VAL_TUPLE_S"` "
                    "but got type `%s` instead"DOC_NEW_TIMER,
                    ti_val_str(query->rval));
            goto fail2;
        }

        n = VARR(query->rval)->n;
        if (n > m)
        {
            ex_set(e, EX_NUM_ARGUMENTS,
                    "got %zu timer argument%s while the given closure "
                    "accepts no more than %zu argument%s"DOC_NEW_TIMER,
                    n, n == 1 ? "" : "s",
                    m, m == 1 ? "" : "s");
            goto fail2;
        }

        if (!query->collection &&
            ti_timer_check_thingsdb_args(VARR(query->rval), e))
            goto fail2;

        for (vec_each(VARR(query->rval), ti_val_t, v), --m)
        {
            VEC_push(args, v);
            ti_incref(v);
        }

        ti_val_drop(query->rval);
        query->rval = NULL;
    }

    while (m--)
        VEC_push(args, ti_nil_get());

    timer_id = ti_vint_create((int64_t) ti_next_free_id());
    if (!timer_id)
        goto fail2;

    if (args->n)
    {
        (void) vec_set(args, timer_id, 0);
        ti_incref(timer_id);
    }

    timer = ti_timer_create(
            VINT(timer_id),
            (uint64_t) start_ts,
            repeat,
            scope_id,
            query->user,
            closure,
            args);
    if (!timer)
    {
        ex_set_mem(e);
        goto fail3;
    }

    VEC_push(*timers, timer);

    task = ti_task_get_task(
            query->change,
            query->collection ? query->collection->root : ti.thing0);

    if (!task || ti_task_add_new_timer(task, timer))
        goto undo;

    query->rval = (ti_val_t *) timer_id;
    goto done;

undo:
    ex_set_mem(e);
    ti_timer_unsafe_drop(VEC_pop(*timers));
fail3:
    ti_val_unsafe_drop((ti_val_t *) timer_id);
fail2:
    vec_destroy(args, (vec_destroy_cb) ti_val_drop);
done:
fail1:
    ti_val_unsafe_drop((ti_val_t *) closure);
    return e->nr;
}
