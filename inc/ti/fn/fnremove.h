#include <ti/fn/fn.h>

#define REMOVE_LIST_DOC_ TI_SEE_DOC("#remove-list")
#define REMOVE_SET_DOC_ TI_SEE_DOC("#remove-set")

static void do__f_remove_list(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    size_t idx = 0;
    ti_closure_t * closure;
    ti_varr_t * varr;
    ti_chain_t chain;

    ti_chain_move(&chain, &query->chain);

    if (nargs < 1)
    {
        ex_set(e, EX_BAD_DATA,
                "function `remove` requires at least 1 argument but 0 "
                "were given"REMOVE_LIST_DOC_);
        goto fail0;
    }

    if (nargs > 2)
    {
        ex_set(e, EX_BAD_DATA,
                "function `remove` takes at most 2 arguments but %d "
                "were given"REMOVE_LIST_DOC_, nargs);
        goto fail0;
    }

    if (ti_val_try_lock(query->rval, e))
        goto fail0;

    varr = (ti_varr_t *) query->rval;
    query->rval = NULL;

    if (ti_do_scope(query, nd->children->node, e))
        goto fail1;

    if (!ti_val_is_closure(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `remove` expects argument 1 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead"
                REMOVE_LIST_DOC_, ti_val_str(query->rval));
        goto fail1;
    }

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (    ti_closure_try_wse(closure, query, e) ||
            ti_closure_try_lock_and_use(closure, query, e))
        goto fail2;

    for (vec_each(varr->vec, ti_val_t, v), ++idx)
    {
        if (ti_closure_vars_val_idx(closure, v, idx))
        {
            ex_set_mem(e);
            goto fail3;
        }

        if (ti_do_scope(query, ti_closure_scope(closure), e))
            goto fail3;

        if (ti_val_as_bool(query->rval))
        {
            ti_val_drop(query->rval);
            query->rval = v;  /* we can move the reference here */

            (void) vec_remove(varr->vec, idx);

            if (ti_chain_is_set(&chain))
            {
                ti_task_t * task;
                task = ti_task_get_task(query->ev, chain.thing, e);
                if (!task)
                    goto fail3;

                if (ti_task_add_splice(
                        task,
                        chain.name,
                        NULL,
                        idx,
                        1,
                        0))
                    ex_set_mem(e);
            }

            goto done;
        }

        ti_val_drop(query->rval);
        query->rval = NULL;
    }

    assert (query->rval == NULL);
    if (nargs == 2)
    {
        /* lazy evaluation of the alternative value */
        (void) ti_do_scope(query, nd->children->next->next->node, e);
    }
    else
        query->rval = (ti_val_t *) ti_nil_get();

done:
fail3:
    ti_closure_unlock_use(closure, query);

fail2:
    ti_val_drop((ti_val_t *) closure);

fail1:
    ti_val_unlock((ti_val_t *) varr, true  /* lock was set */);
    ti_val_drop((ti_val_t *) varr);
fail0:
    ti_chain_unset(&chain);
}

static int do__f_remove_set_from_closure(
        vec_t ** removed,
        ti_vset_t * vset,
        ti_query_t * query,
        const int nargs,
        ex_t * e)
{
    ti_closure_t * closure = (ti_closure_t *) query->rval;
    vec_t * vec = imap_vec(vset->imap);
    query->rval = NULL;
    if (!vec)
    {
        ex_set_mem(e);
        goto fail1;
    }

    if (nargs > 1)
    {
        ex_set(e, EX_BAD_DATA,
                "function `remove` takes at most 1 argument when using a `"
                TI_VAL_CLOSURE_S"` but %d were given"REMOVE_SET_DOC_,
                nargs);
        goto fail1;
    }

    if (    ti_closure_try_wse(closure, query, e) ||
            ti_closure_try_lock_and_use(closure, query, e))
        goto fail1;

    for (vec_each(vec, ti_thing_t, t))
    {
        if (ti_closure_vars_val_idx(closure, (ti_val_t *) t, t->id))
        {
            ex_set_mem(e);
            goto fail2;
        }

        if (ti_do_scope(query, ti_closure_scope(closure), e))
            goto fail2;

        if (ti_val_as_bool(query->rval) && vec_push(removed, t))
        {
            ex_set_mem(e);
            goto fail2;
        }

        ti_val_drop(query->rval);
        query->rval = NULL;
    }

fail2:
    ti_closure_unlock_use(closure, query);

fail1:
    ti_val_drop((ti_val_t *) closure);
    return e->nr;
}

static void do__f_remove_set(
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    vec_t * removed = NULL;
    const int nargs = langdef_nd_n_function_params(nd);
    ti_vset_t * vset;
    ti_chain_t chain;

    ti_chain_move(&chain, &query->chain);

    if (nargs < 1)
    {
        ex_set(e, EX_BAD_DATA,
                "function `remove` requires at least 1 argument but 0 "
                "were given"REMOVE_SET_DOC_);
        goto fail0;
    }

    if (ti_val_try_lock(query->rval, e))
        goto fail0;

    vset = (ti_vset_t *) query->rval;
    query->rval = NULL;

    if (ti_do_scope(query, nd->children->node, e))
        goto fail1;

    if (ti_val_is_closure(query->rval))
    {
        removed = vec_new(1);
        if (!removed)
        {
            ex_set_mem(e);
            goto fail1;
        }

        if (do__f_remove_set_from_closure(&removed, vset, query, nargs, e))
            goto fail2;

        assert (query->rval == NULL);

        for (vec_each(removed, ti_thing_t, t))
            (void *) ti_vset_pop(vset, t);
    }
    else
    {
        size_t narg;
        cleri_children_t * child = nd->children;

        removed = vec_new(nargs);
        if (!removed)
        {
            ex_set_mem(e);
            goto fail1;
        }

        for (narg = 1;;++narg)
        {
            if (!ti_val_is_thing(query->rval))
            {
                ex_set(e, EX_BAD_DATA,
                        narg == 1
                        ?
                        "function `remove` expects argument %d to be "
                        "a `"TI_VAL_CLOSURE_S"` or type `"TI_VAL_THING_S"` "
                        "but got type `%s` instead"REMOVE_SET_DOC_
                        :
                        "function `remove` expects argument %d to be "
                        "of type `"TI_VAL_THING_S"` "
                        "but got type `%s` instead"REMOVE_SET_DOC_,
                        narg, ti_val_str(query->rval));

                goto fail2;
            }

            if (ti_vset_pop(vset, (ti_thing_t *) query->rval))
                VEC_push(removed, query->rval);

            ti_val_drop(query->rval);
            query->rval = NULL;

            if (!child->next || !(child = child->next->next))
                break;

            if (ti_do_scope(query, child->node, e))
                goto fail2;
        }
    }

    if (removed->n && ti_chain_is_set(&chain))
    {
        ti_task_t * task = ti_task_get_task(query->ev, chain.thing, e);
        if (!task)
            goto fail2;

        if (ti_task_add_remove(
                task,
                chain.name,
                removed))
        {
            ex_set_mem(e);
            goto fail2;
        }
    }

    assert (query->rval == NULL);
    query->rval = (ti_val_t *) ti_varr_from_vec(removed);
    if (query->rval)
        goto done;

    ex_set_mem(e);

fail2:
    while (removed->n)
        /* if the `thing` is already in the set, then the set still owns the
         * thing, else the thing was owned by the `removed` vector so in
         * neither case we have to adjust the reference counter */
        if (ti_vset_add(vset, vec_pop(removed)) == IMAP_ERR_ALLOC)
            ex_set_mem(e);
    free(removed);

fail1:
done:
    ti_val_unlock((ti_val_t *) vset, true  /* lock was set */);
    ti_val_drop((ti_val_t *) vset);
fail0:
    ti_chain_unset(&chain);
}

static int do__f_remove(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);


    if (ti_val_is_list(query->rval))
        do__f_remove_list(query, nd, e);
    else if (ti_val_is_set(query->rval))
        do__f_remove_set(query, nd, e);
    else
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `remove`"
                REMOVE_LIST_DOC_ REMOVE_SET_DOC_,
                ti_val_str(query->rval));

    return e->nr;
}
