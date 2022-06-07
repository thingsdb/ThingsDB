#include <ti/fn/fn.h>

static int do__f_args(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_vtask_t * vtask;
    vec_t * args;
    size_t i, n;

    if (!ti_val_is_task(query->rval))
        return fn_call_try("args", query, nd, e);

    vtask = (ti_vtask_t *) query->rval;
    if (ti_vtask_is_nil(vtask, e))
        return e->nr;

    query->rval = NULL;

    if (fn_nargs("args", DOC_TASK_ARGS, 0, nargs, e))
        goto fail0;

    n = vtask->args->n ? vtask->args->n-1 : 0;

    args = vec_new(n++);
    if (!args)
    {
        ex_set_mem(e);
        goto fail0;
    }

    for (i=1; i < n; ++i)
    {
        ti_val_t * v = VEC_get(vtask->args, i);
        VEC_push(args, v);
        ti_incref(v);
    }

    /*
     * All the args were tuple member to begin with, thus it is safe to create
     * a tuple from these arguments.
     */
    query->rval = (ti_val_t *) ti_tuple_from_vec_unsafe(args);
    if (!query->rval)
    {
        ex_set_mem(e);
        vec_destroy(args, (vec_destroy_cb) ti_val_unsafe_drop);
    }

fail0:
    ti_vtask_unsafe_drop(vtask);
    return e->nr;
}
