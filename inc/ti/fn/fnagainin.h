#include <ti/fn/fn.h>

static int do__f_again_in(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_tz_t * tz = query->collection ? query->collection->tz : ti_tz_utc();
    ti_vtask_t * vtask;
    time_t again_at;
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
        ti_do_statement(query, nd->children->node, e) ||
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

    if (ti_do_statement(query, nd->children->next->next->node, e) ||
        fn_arg_int("again_in", DOC_TASK_AGAIN_IN, 2, query->rval, e))
        goto fail1;

    num = VINT(query->rval);

    ti_val_unsafe_drop(query->rval);  /* this destroys `integer value` */
    query->rval = (ti_val_t *) ti_nil_get();

    (void) ti_datetime_move(dt, unit, num, e);

    again_at = DATETIME(dt);
    if ((uint64_t) again_at < vtask->run_at + TI_TASKS_MIN_REPEAT)
    {
        ex_set(e, EX_VALUE_ERROR,
                "new start time must be at least one minute (%d seconds) "
                "after the previous start time"DOC_TASK_AGAIN_IN,
                TI_TASKS_MIN_REPEAT);
        goto fail1;
    }

    if (again_at > UINT32_MAX)
    {
        ex_set(e, EX_VALUE_ERROR, "start time out-of-range"DOC_TASK_AGAIN_IN);
        goto fail1;
    }

    ti_vtask_again_at(vtask, (uint64_t) again_at);

fail1:
    ti_val_unsafe_drop((ti_val_t *) dt);
fail0:
    ti_vtask_unlock(vtask);
    ti_vtask_unsafe_drop(vtask);
    return e->nr;
}
