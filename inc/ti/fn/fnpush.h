#include <ti/fn/fn.h>

#define PUSH_DOC_ TI_SEE_DOC("#push")

static int do__f_push(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    cleri_children_t * child = nd->children;    /* first in argument list */
    uint32_t current_n, new_n;
    ti_varr_t * varr;
    ti_chain_t chain;

    if (fn_not_chained("push", query, e))
        return e->nr;

    ti_chain_move(&chain, &query->chain);

    if (!ti_val_is_list(query->rval))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `push`"PUSH_DOC_,
                ti_val_str(query->rval));
        goto fail0;
    }

    if (!nargs)
    {
        ex_set(e, EX_BAD_DATA,
                "function `push` requires at least 1 argument but 0 "
                "were given"PUSH_DOC_);
        goto fail0;
    }

    if (ti_val_try_lock(query->rval, e))
        goto fail0;

    varr = (ti_varr_t *) query->rval;
    query->rval = NULL;

    current_n = varr->vec->n;
    new_n = current_n + nargs;

    if (query->target && new_n >= query->target->quota->max_array_size)
    {
        ex_set(e, EX_MAX_QUOTA,
                "maximum array size quota of %zu has been reached"
                TI_SEE_DOC("#quotas"), query->target->quota->max_array_size);
        goto done;
    }

    if (vec_resize(&varr->vec, new_n))
    {
        ex_set_mem(e);
        goto done;
    }

    do
    {
        if (ti_do_scope(query, child->node, e) ||
            ti_varr_append(varr, (void **) &query->rval, e))
            goto fail1;

        query->rval = NULL;
    }
    while (child->next && (child = child->next->next));

    if (ti_chain_is_set(&chain))
    {
        ti_task_t * task = ti_task_get_task(query->ev, chain.thing, e);
        if (!task)
            goto fail1;

        if (ti_task_add_splice(
                task,
                chain.name,
                varr,
                current_n,
                0,
                nargs))
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
    {
        ti_val_drop(vec_pop(varr->vec));
    }
    (void) vec_shrink(&varr->vec);

done:
    ti_val_unlock((ti_val_t *) varr, true  /* lock was set */);
    ti_val_drop((ti_val_t *) varr);

fail0:
    ti_chain_unset(&chain);
    return e->nr;
}
