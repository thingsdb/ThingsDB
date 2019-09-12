#include <ti/fn/fn.h>

#define EXTEND_DOC_ TI_SEE_DOC("#extend")

static int do__f_extend(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    uint32_t current_n, source_n;
    ti_varr_t * varr_dest, * varr_source;
    ti_chain_t chain;

    if (fn_not_chained("extend", query, e))
        return e->nr;

    ti_chain_move(&chain, &query->chain);

    if (!ti_val_is_list(query->rval))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `extend`"EXTEND_DOC_,
                ti_val_str(query->rval));
        goto fail0;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `extend` takes 1 argument but %d were given"
                EXTEND_DOC_, nargs);
        goto fail0;
    }

    if (ti_val_try_lock(query->rval, e))
        goto fail0;

    varr_dest = (ti_varr_t *) query->rval;
    query->rval = NULL;

    current_n = varr_dest->vec->n;

    if (ti_do_statement(query, nd->children->node, e))
        goto fail1;

    if (!ti_val_is_list(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `extend` expects argument 1 to be of "
                "type `"TI_VAL_ARR_S"` but got type `%s` instead"EXTEND_DOC_,
                ti_val_str(query->rval));
        goto fail1;
    }

    varr_source = (ti_varr_t *) query->rval;
    query->rval = NULL;

    source_n = varr_source->vec->n;

    if (query->collection &&
        (source_n + current_n) >= query->collection->quota->max_array_size)
    {
        ex_set(e, EX_MAX_QUOTA,
                "maximum array size quota of %zu has been reached"
                TI_SEE_DOC("#quotas"),
                query->collection->quota->max_array_size);
        goto fail2;
    }

    if (vec_extend(&varr_dest->vec, varr_source->vec->data, source_n))
    {
        ex_set_mem(e);
        goto fail2;
    }

    /*
     * If this is the last one we can just clear the source and otherwise it is
     * required to increase the references of each value
     */
    if (varr_source->ref == 1)
        vec_clear(varr_source->vec);
    else for (vec_each(varr_source->vec, ti_val_t, v))
        ti_incref(v);

    if (ti_chain_is_set(&chain))
    {
        ti_task_t * task = ti_task_get_task(query->ev, chain.thing, e);
        if (!task)
            goto fail3;

        if (ti_task_add_splice(
                task,
                chain.name,
                varr_dest,
                current_n,
                0,
                source_n))
            goto alloc_err;  /* we do not need to cleanup task, since the task
                                is added to `query->ev->tasks` */
    }

    assert (e->nr == 0);
    query->rval = (ti_val_t *) ti_vint_create((int64_t) varr_dest->vec->n);
    if (query->rval)
        goto done;

alloc_err:
    ex_set_mem(e);

fail3:
    while (varr_dest->vec->n > current_n)
    {
        ti_val_drop(vec_pop(varr_dest->vec));
    }
    (void) vec_shrink(&varr_dest->vec);

done:
fail2:
    ti_val_drop((ti_val_t *) varr_source);

fail1:
    ti_val_unlock((ti_val_t *) varr_dest, true  /* lock was set */);
    ti_val_drop((ti_val_t *) varr_dest);

fail0:
    ti_chain_unset(&chain);
    return e->nr;
}
