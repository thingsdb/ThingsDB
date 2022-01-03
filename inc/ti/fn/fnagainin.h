#include <ti/fn/fn.h>

static int do__f_again_in(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    time_t run_at, now = util_now_tsec();
    ti_tz_t * tz = fn_default_tz(query);
    ti_vtask_t * vtask;
    ti_datetime_t * dt;
    datetime_unit_e unit;
    int64_t num;

    if (!ti_val_is_task(query->rval))
        return fn_call_try("again_in", query, nd, e);

    vtask = (ti_vtask_t *) query->rval;
    if (ti_vtask_lock(vtask, e))
        return e->nr;

    query->rval = NULL;

    if (ti_query_task_context(query, vtask, e) ||
        fn_nargs("again_in", DOC_TASK_AGAIN_IN, 2, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_str("again_in", DOC_TASK_AGAIN_IN, 1, query->rval, e))
        goto fail0;

    dt = ti_datetime_from_u64(vtask->run_at, tz);
    if (!dt)
        goto fail0;

    unit = ti_datetime_get_unit((ti_raw_t *) query->rval, e);
    if (e->nr)
        goto fail1;

    ti_val_unsafe_drop(query->rval);  /* this destroys `raw_unit` */
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next, e) ||
        fn_arg_int("again_in", DOC_TASK_AGAIN_IN, 2, query->rval, e))
        goto fail1;

    num = VINT(query->rval);

    ti_val_unsafe_drop(query->rval);  /* this destroys `integer value` */
    query->rval = (ti_val_t *) ti_nil_get();

    if (ti_datetime_move(dt, unit, num, e))
        goto fail1;

    run_at = DATETIME(dt);

    /*
     * When using again-in, we want the new time to be relative to the last
     * run_at time so we can nicely move a task exactly a month, day, etc.
     *
     * Especially if we use a low value, it might happen that the new time
     * does not exceed the current time. In this case we can do extra `steps`
     * to reach at least the current time but we should at least step towards
     * the future.
     */
    if (run_at < now && run_at > (int64_t) vtask->run_at)
    {
        do
        {
            if (ti_datetime_move(dt, unit, num, e))
                goto fail1;
            run_at = DATETIME(dt);
        }
        while (run_at < now);
    }

    if (run_at < now || run_at > UINT32_MAX)
    {
        ex_set(e, EX_VALUE_ERROR, "start time out-of-range"DOC_TASK);
        goto fail1;
    }

    ti_vtask_again_at(vtask, (uint64_t) run_at);

fail1:
    ti_val_unsafe_drop((ti_val_t *) dt);
fail0:
    ti_vtask_unlock(vtask);
    ti_vtask_unsafe_drop(vtask);
    return e->nr;
}
