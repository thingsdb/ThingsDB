#include <ti/cfn/fn.h>

#define REMOVE_DOC_ TI_SEE_DOC("#remove")

static int cq__f_remove(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool found;
    const int nargs = langdef_nd_n_function_params(nd);
    size_t idx = 0;
    ti_closure_t * closure = NULL;
    ti_varr_t * varr = (ti_varr_t *) ti_query_val_pop(query);

    if (!ti_val_is_list((ti_val_t *) varr))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `remove`"REMOVE_DOC_,
                ti_val_str((ti_val_t *) varr));
        goto done;
    }

    if (nargs < 1)
    {
        ex_set(e, EX_BAD_DATA,
                "function `remove` requires at least 1 argument but 0 "
                "were given"REMOVE_DOC_);
        goto done;
    }

    if (nargs > 2)
    {
        ex_set(e, EX_BAD_DATA,
                "function `remove` takes at most 2 arguments but %d "
                "were given"REMOVE_DOC_, nargs);
        goto done;
    }

    if (ti_varr_is_assigned(varr) &&
        ti_scope_in_use_val(query->scope->prev, (ti_val_t *) varr))
    {
        ex_set(e, EX_BAD_DATA,
                "cannot use function `remove` while the list is in use"
                REMOVE_DOC_);
        goto done;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        goto done;

    if (!ti_val_is_closure(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `remove` expects argument 1 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead"REMOVE_DOC_,
                ti_val_str(query->rval));
        goto done;
    }

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (ti_closure_try_lock(closure, e) ||
        ti_scope_local_from_closure(query->scope, closure, e))
        goto done;

    for (vec_each(varr->vec, ti_val_t, v), ++idx)
    {
        if (ti_scope_polute_val(query->scope, v, idx))
        {
            ex_set_alloc(e);
            goto done;
        }

        if (ti_cq_optscope(query, ti_closure_scope_nd(closure), e))
            goto done;

        found = ti_val_as_bool(query->rval);
        ti_val_drop(query->rval);

        if (found)
        {
            query->rval = v;  /* we can move the reference here */
            (void) vec_remove(varr->vec, idx);

            if (ti_varr_is_assigned(varr))
            {
                ti_task_t * task;
                assert (query->scope->thing);
                assert (query->scope->name);
                task = ti_task_get_task(query->ev, query->scope->thing, e);
                if (!task)
                    goto done;

                if (ti_task_add_splice(
                        task,
                        query->scope->name,
                        NULL,
                        idx,
                        1,
                        0))
                    ex_set_alloc(e);
            }

            goto done;
        }
        query->rval = NULL;
    }

    assert (query->rval == NULL);

    if (nargs == 2)
    {
        /* lazy evaluation of the alternative value */
        (void) ti_cq_scope(query, nd->children->next->next->node, e);
    }
    else
    {
        query->rval = (ti_val_t *) ti_nil_get();
    }

done:
    ti_closure_unlock(closure);
    ti_val_drop((ti_val_t *) closure);
    ti_val_drop((ti_val_t *) varr);
    return e->nr;
}
