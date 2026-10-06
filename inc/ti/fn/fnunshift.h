#include <ti/fn/fn.h>

static int do__f_unshift(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    cleri_node_t * child = nd->children;    /* first in argument list */
    uint32_t current_n, new_n, idx = 0;
    ti_varr_t * varr;

    if (!ti_val_is_list(query->rval))
        return fn_call_try("unshift", query, nd, e);

    if (fn_nargs_min("unshift", DOC_LIST_UNSHIFT, 1, nargs, e) ||
        ti_query_test_varr_operation(query, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    varr = (ti_varr_t *) query->rval;
    query->rval = NULL;

    current_n = varr->vec->n;
    new_n = current_n + nargs;

    if (vec_reserve(&varr->vec, new_n))
    {
        ex_set_mem(e);
        goto done;
    }

    do
    {
        /*
         * TODO (Performance)
         * Technically, the insert performance could be improved by replacing
         * the `ti_varr_insert` with an optimized function to insert a bulk
         * at once. This is especially useful for the insert function since
         * the current solution requires a call to memmove() on each iteration.
         */
        if (ti_do_statement(query, child, e) ||
            ti_val_varr_insert(varr, &query->rval, e, idx))
            goto fail1;
        ++idx;
        query->rval = NULL;
    }
    while (child->next && (child = child->next->next));

    if (varr->parent && varr->parent->id)
    {
        ti_task_t * task = ti_task_get_task(query->change, varr->parent);
        if (!task || ti_task_add_splice(
                task,
                ti_varr_key(varr),
                varr,
                0,
                0,
                (uint32_t) nargs))
            goto alloc_err;  /* we do not need to cleanup task, since the task
                                is added to `query->change->tasks` */
    }

    assert(e->nr == 0);
    query->rval = (ti_val_t *) ti_vint_create((int64_t) varr->vec->n);
    if (query->rval)
        goto done;

alloc_err:
    ex_set_mem(e);

fail1:
    while (varr->vec->n > current_n)
        ti_val_unsafe_drop(vec_remove(varr->vec, 0));

    (void) vec_may_shrink(&varr->vec);

done:
    ti_val_unlock((ti_val_t *) varr, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) varr);
    return e->nr;
}
