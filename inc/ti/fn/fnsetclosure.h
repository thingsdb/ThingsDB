#include <ti/fn/fn.h>

static int do__f_set_closure(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_task_t * task;
    ti_vtask_t * vtask;
    ti_closure_t * closure;

    if (!ti_val_is_task(query->rval))
        return fn_call_try("set_closure", query, nd, e);

    vtask = (ti_vtask_t *) query->rval;
    if (ti_vtask_lock(vtask, e))
        return e->nr;

    query->rval = NULL;

    if (fn_not_thingsdb_or_collection_scope("set_closure", query, e) ||
        fn_nargs("set_closure", DOC_TASK_SET_CLOSURE, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_closure("set_closure", DOC_TASK_SET_CLOSURE, 1, query->rval, e))
        goto fail0;

    closure = (ti_closure_t *) query->rval;
    if (ti_closure_unbound(closure, e))
        goto fail0;

    if (closure != vtask->closure)
    {
        if (ti_vtask_set_closure(vtask, closure))
        {
            ex_set_mem(e);
            goto fail0;
        }

        ti_incref(closure);

        task = ti_task_get_task(
                query->change,
                query->collection ? query->collection->root : ti.thing0);

        if (!task || ti_task_add_vtask_set_args(task, vtask))
            ex_set_mem(e);  /* task cleanup is not required */
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

fail0:
    ti_vtask_unlock(vtask);
    ti_vtask_unsafe_drop(vtask);
    return e->nr;
}
