#include <ti/fn/fn.h>

static int do__f_extend(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    uint32_t current_n, source_n;
    ti_varr_t * varr_dest, * varr_source;
    ti_chain_t chain;

    if (!ti_val_is_list(query->rval))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no function `extend`",
                ti_val_str(query->rval));
        return e->nr;
    }

    if (fn_nargs("extend", DOC_LIST_EXTEND, 1, nargs, e))
        return e->nr;

    ti_chain_move(&chain, &query->chain);

    if (ti_val_try_lock(query->rval, e))
        goto fail0;

    varr_dest = (ti_varr_t *) query->rval;
    query->rval = NULL;

    current_n = varr_dest->vec->n;

    if (ti_do_statement(query, nd->children->node, e))
        goto fail1;

    if (!ti_val_is_list(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `extend` expects argument 1 to be of "
                "type `"TI_VAL_LIST_S"` but got type `%s` instead"
                DOC_LIST_EXTEND,
                ti_val_str(query->rval));
        goto fail1;
    }

    varr_source = (ti_varr_t *) query->rval;
    query->rval = NULL;

    source_n = varr_source->vec->n;

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
