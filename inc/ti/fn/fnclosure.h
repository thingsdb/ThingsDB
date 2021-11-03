#include <ti/fn/fn.h>

static int do__f_closure(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_vtask_t * vtask;

    if (!ti_val_is_task(query->rval))
        return fn_call_try("closure", query, nd, e);

    vtask = (ti_vtask_t *) query->rval;

    if (ti_vtask_is_nil(vtask, e) ||
        fn_nargs("closure", DOC_TASK_CLOSURE, 0, nargs, e))
        return e->nr;

    query->rval = (ti_val_t *) vtask->closure;
    ti_incref(query->rval);

    ti_vtask_unsafe_drop(vtask);
    return e->nr;
}
