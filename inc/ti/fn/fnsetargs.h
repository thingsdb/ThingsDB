#include <ti/fn/fn.h>

static int do__f_set_args(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_task_t * task;
    ti_vtask_t * vtask;
    size_t m, idx = 1;

    if (!ti_val_is_task(query->rval))
        return fn_call_try("again_at", query, nd, e);

    vtask = (ti_vtask_t *) query->rval;
    if (ti_vtask_lock(vtask, e))
        return e->nr;

    query->rval = NULL;

    m = vtask->closure->vars->n;

    if (fn_not_thingsdb_or_collection_scope("set_args", query, e) ||
        fn_nargs("set_args", DOC_TASK_SET_ARGS, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_array("set_args", DOC_TASK_SET_ARGS, 1, query->rval, e) ||
        ti_vtask_check_args(VARR(query->rval), m, query->collection==NULL, e))
        goto fail0;

    for (vec_each(VARR(query->rval), ti_val_t, v), ++idx)
    {
        /*
         * No garbage collection safe drop is required as all the possible
         * things are guaranteed to have Id's.
         */
        ti_val_unsafe_drop(vec_set(vtask->args, v, idx));
        ti_incref(v);
    }

    while (idx < m)
        ti_val_unsafe_drop(vec_set(vtask->args, ti_nil_get(), idx++));

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    task = ti_task_get_task(
            query->change,
            query->collection ? query->collection->root : ti.thing0);

    if (!task || ti_task_add_vtask_set_args(task, vtask))
        ex_set_mem(e);  /* task cleanup is not required */

fail0:
    ti_vtask_unlock(vtask);
    ti_vtask_unsafe_drop(vtask);
    return e->nr;
}
