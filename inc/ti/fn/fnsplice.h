#include <ti/fn/fn.h>

static int do__f_splice(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    int32_t n, x, l;
    cleri_children_t * child = nd->children;    /* first in argument list */
    uint32_t current_n, new_n;
    int64_t i, c;
    ti_varr_t * retv;
    ti_varr_t * varr;
    ti_chain_t chain;

    if (fn_not_chained("splice", query, e))
        return e->nr;

    if (!ti_val_is_list(query->rval))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no function `splice`",
                ti_val_str(query->rval));
        return e->nr;
    }

    ti_chain_move(&chain, &query->chain);

    n = langdef_nd_n_function_params(nd);
    if (fn_nargs_min("splice", DOC_LIST_SPLICE, 2, n, e) ||
        ti_val_try_lock(query->rval, e))
        goto fail0;

    varr = (ti_varr_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, child->node, e))
        goto fail1;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `splice` expects argument 1 to be of "
                "type `"TI_VAL_INT_S"` but got type `%s` instead"
                DOC_LIST_SPLICE,
                ti_val_str(query->rval));
        goto fail1;
    }

    i = ((ti_vint_t *) query->rval)->int_;
    ti_val_drop(query->rval);
    query->rval = NULL;
    child = child->next->next;

    if (ti_do_statement(query, child->node, e))
        goto fail1;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `splice` expects argument 2 to be of "
                "type `"TI_VAL_INT_S"` but got type `%s` instead"
                DOC_LIST_SPLICE,
                ti_val_str(query->rval));
        goto fail1;
    }

    c = ((ti_vint_t *) query->rval)->int_;
    current_n = varr->vec->n;

    ti_val_drop(query->rval);
    query->rval = NULL;

    if (i < 0)
        i += current_n;

    i = i < 0 ? (c += i) && 0 : (i > current_n ? current_n : i);
    n -= 2;
    c = c < 0 ? 0 : (c > current_n - i ? current_n - i : c);
    new_n = current_n + n - c;

    if (query->collection && new_n >= query->collection->quota->max_array_size)
    {
        ex_set(e, EX_MAX_QUOTA,
                "maximum array size quota of %zu has been reached"
                DOC_SET_QUOTA,
                query->collection->quota->max_array_size);
        goto fail1;
    }

    if (new_n > current_n && vec_resize(&varr->vec, new_n))
    {
        ex_set_mem(e);
        goto fail1;
    }

    retv = ti_varr_create(c);
    if (!retv)
    {
        ex_set_mem(e);
        goto fail1;
    }

    for (x = i, l = i + c; x < l; ++x)
        VEC_push(retv->vec, vec_get(varr->vec, x));

    memmove(
        varr->vec->data + i + n,
        varr->vec->data + i + c,
        (current_n - i - c) * sizeof(void*));

    varr->vec->n = i;

    for (x = 0; x < n; ++x)
    {
        child = child->next->next;

        if (ti_do_statement(query, child->node, e) ||
            ti_varr_append(varr, (void **) &query->rval, e))
            goto fail2;

        query->rval = NULL;
    }

    if (ti_chain_is_set(&chain))
    {
        ti_task_t * task = ti_task_get_task(query->ev, chain.thing, e);
        if (!task)
            goto fail2;

        if (ti_task_add_splice(
                task,
                chain.name,
                varr,
                (uint32_t) i,
                (uint32_t) c,
                (uint32_t) n))
            goto alloc_err;  /* we do not need to cleanup task, since the task
                                is added to `query->ev->tasks` */
    }

    assert (e->nr == 0);

    /* required since query->rval may not be NULL */
    ti_val_drop(query->rval);

    query->rval = (ti_val_t *) retv;
    varr->vec->n = new_n;
    if (new_n < current_n)
        (void) vec_shrink(&varr->vec);
    goto done;

alloc_err:
    ex_set_mem(e);

fail2:
    while (x--)
        ti_val_drop(vec_pop(varr->vec));

    memmove(
        varr->vec->data + i + n,
        varr->vec->data + i + c,
        (current_n - i - c) * sizeof(void*));

    for (x = 0; x < c; ++x)
        VEC_push(varr->vec, vec_get(retv->vec, x));

    retv->vec->n = 0;
    ti_val_drop((ti_val_t *) retv);
    varr->vec->n = current_n;

done:
fail1:
    ti_val_unlock((ti_val_t *) varr, true  /* lock was set */);
    ti_val_drop((ti_val_t *) varr);
fail0:
    ti_chain_unset(&chain);
    return e->nr;
}
