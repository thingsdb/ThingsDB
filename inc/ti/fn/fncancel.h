#include <ti/fn/fn.h>

static int do__f_cancel(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_task_t * task;
    ti_vtask_t * vtask;

    if (!ti_val_is_task(query->rval))
        return fn_call_try("cancel", query, nd, e);

    vtask = (ti_vtask_t *) query->rval;
    if (ti_vtask_is_locked(vtask, e))
        return e->nr;

    query->rval = (ti_val_t *) ti_nil_get();;

    if (fn_nargs("cancel", DOC_TASK_CANCEL, 0, nargs, e))
        goto fail0;

    if (vtask->run_at)
    {
        task = ti_task_get_task(
                query->change,
                query->collection ? query->collection->root : ti.thing0);

        if (task && ti_task_add_vtask_cancel(task, vtask) == 0)
            vtask->run_at = 0;
        else
            ex_set_mem(e);
    }

fail0:
    ti_vtask_unsafe_drop(vtask);
    return e->nr;
}
