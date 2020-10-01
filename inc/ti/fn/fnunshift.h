#include <ti/fn/fn.h>

static int do__f_unshift(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    cleri_children_t * child = nd->children;    /* first in argument list */
    uint32_t current_n, new_n, idx = 0;
    ti_varr_t * varr;

    if (!ti_val_is_list(query->rval))
        return fn_call_try("unshift", query, nd, e);

    if (fn_nargs_min("unshift", DOC_LIST_UNSHIFT, 1, nargs, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    varr = (ti_varr_t *) query->rval;
    query->rval = NULL;

    current_n = varr->vec->n;
    new_n = current_n + nargs;

    if (vec_resize(&varr->vec, new_n))
    {
        ex_set_mem(e);
        goto done;
    }

    do
    {
        /*
         * Technically the insert performance could be improved by replacing
         * the `ti_varr_insert` by a single memmove() function call, followed
         * by a ti_varr_val_prepare() and vec_set() function call for each
         * value.
         */
        if (ti_do_statement(query, child->node, e) ||
            ti_varr_insert(varr, (void **) &query->rval, e, idx))
            goto fail1;
        ++idx;
        query->rval = NULL;
    }
    while (child->next && (child = child->next->next));

    if (varr->parent && varr->parent->id)
    {
        ti_task_t * task = ti_task_get_task(query->ev, varr->parent);
        if (!task || ti_task_add_splice(
                task,
                varr->name,
                varr,
                0,
                0,
                (uint32_t) nargs))
            goto alloc_err;  /* we do not need to cleanup task, since the task
                                is added to `query->ev->tasks` */
    }

    assert (e->nr == 0);
    query->rval = (ti_val_t *) ti_vint_create((int64_t) varr->vec->n);
    if (query->rval)
        goto done;

alloc_err:
    ex_set_mem(e);

fail1:
    while (varr->vec->n > current_n)
        ti_val_unsafe_drop(vec_remove(varr->vec, 0));

    (void) vec_shrink(&varr->vec);

done:
    ti_val_unlock((ti_val_t *) varr, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) varr);
    return e->nr;
}
