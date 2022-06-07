#include <ti/fn/fn.h>

static int do__f_add(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    cleri_node_t * child = nd->children;        /* first in argument list */
    vec_t * added;
    ti_vset_t * vset;

    if (!ti_val_is_set(query->rval))
        return fn_call_try("add", query, nd, e);

    added = vec_new(nargs);  /* weak references to things */
    if (!added)
    {
        ex_set_mem(e);
        goto fail0;
    }

    if (fn_nargs_min("add", DOC_SET_ADD, 1, nargs, e) ||
        ti_val_try_lock(query->rval, e))
        goto fail0;

    vset = (ti_vset_t *) query->rval;
    query->rval = NULL;

    do
    {
        int rc;
        assert (child->cl_obj->gid == CLERI_GID_STATEMENT);

        if (ti_do_statement(query, child, e))
            goto fail1;

        rc = ti_vset_add_val(vset, query->rval, e);

        if (rc < 0)
            goto fail1;

        if (rc > 0)
            VEC_push(added, query->rval);  /* at least the `set` has a
                                              reference now */
        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }
    while (child->next && (child = child->next->next));

    if (added->n && vset->parent && vset->parent->id)
    {
        ti_task_t * task = ti_task_get_task(query->change, vset->parent);
        if (!task || ti_task_add_set_add(
                task,
                ti_vset_key(vset),
                added))
            goto alloc_err;  /* we do not need to cleanup task, since the task
                                is added to `query->change->tasks` */
    }

    assert (e->nr == 0);

    query->rval = (ti_val_t *) ti_vint_create((int64_t) added->n);
    if (query->rval)
        goto done;

alloc_err:
    ex_set_mem(e);

fail1:
    while (added->n)
        ti_val_unsafe_drop((ti_val_t *) ti_vset_pop(vset, VEC_pop(added)));

done:
    ti_val_unlock((ti_val_t *) vset, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) vset);

fail0:
    free(added);
    return e->nr;
}
