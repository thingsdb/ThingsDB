#include <ti/fn/fn.h>

static int do__f_again_at(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    time_t run_at, now = util_now_tsec();
    ti_vtask_t * vtask;

    if (!ti_val_is_task(query->rval))
        return fn_call_try("again_at", query, nd, e);

    vtask = (ti_vtask_t *) query->rval;
    if (ti_vtask_lock(vtask, e))
        return e->nr;

    query->rval = NULL;

    if (ti_query_task_context(query, vtask, e) ||
        fn_nargs("again_at", DOC_TASK_AGAIN_AT, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_datetime("again_at", DOC_TASK_AGAIN_AT, 1, query->rval, e))
        goto fail0;

    run_at = DATETIME(query->rval);

    if (run_at < now || run_at > UINT32_MAX)
    {
        ex_set(e, EX_VALUE_ERROR, "start time out-of-range"DOC_TASK);
        goto fail0;
    }

    ti_vtask_again_at(vtask, (uint64_t) run_at);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

fail0:
    ti_vtask_unlock(vtask);
    ti_vtask_unsafe_drop(vtask);
    return e->nr;
}
