#include <ti/fn/fn.h>

static int do__f_at(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_tz_t * tz = query->collection ? query->collection->tz : ti_tz_utc();
    ti_vtask_t * vtask;

    if (!ti_val_is_task(query->rval))
        return fn_call_try("at", query, nd, e);

    vtask = (ti_vtask_t *) query->rval;

    if (fn_nargs("at", DOC_TASK_AT, 0, nargs, e))
        return e->nr;

    if (vtask->run_at)
    {
        ti_datetime_t * dt = ti_datetime_from_u64(vtask->run_at, tz);
        query->rval = (ti_val_t *) dt;
        if (!dt)
            ex_set_mem(e);
    }
    else
        query->rval = (ti_val_t *) ti_nil_get();

    ti_vtask_unsafe_drop(vtask);
    return e->nr;
}
