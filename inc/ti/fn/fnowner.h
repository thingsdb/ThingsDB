#include <ti/fn/fn.h>

static int do__f_owner(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_vtask_t * vtask;

    if (!ti_val_is_task(query->rval))
        return fn_call_try("owner", query, nd, e);

    vtask = (ti_vtask_t *) query->rval;

    if (ti_vtask_is_nil(vtask, e) ||
        fn_nargs("owner", DOC_TASK_OWNER, 0, nargs, e))
        return e->nr;

    query->rval = (ti_val_t *) vtask->user->name;
    ti_incref(query->rval);

    ti_vtask_unsafe_drop(vtask);
    return e->nr;
}
