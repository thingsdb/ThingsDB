#include <ti/fn/fn.h>

#define REMOVE_LIST_DOC_ TI_SEE_DOC("#remove-list")
#define REMOVE_SET_DOC_ TI_SEE_DOC("#remove-set")

static void do__f_remove_list(
        ti_varr_t * varr,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    _Bool found;
    const int nargs = langdef_nd_n_function_params(nd);
    size_t idx = 0;
    ti_closure_t * closure = NULL;
    _Bool is_attached = ti_scope_is_attached(query->scope);

    assert (ti_val_is_list((ti_val_t *) varr));

    if (nargs < 1)
    {
        ex_set(e, EX_BAD_DATA,
                "function `remove` requires at least 1 argument but 0 "
                "were given"REMOVE_LIST_DOC_);
        goto fail1;
    }

    if (nargs > 2)
    {
        ex_set(e, EX_BAD_DATA,
                "function `remove` takes at most 2 arguments but %d "
                "were given"REMOVE_LIST_DOC_, nargs);
        goto fail1;
    }

    if (is_attached &&
        ti_scope_in_use_val(query->scope->prev, (ti_val_t *) varr))
    {
        ex_set(e, EX_BAD_DATA,
                "cannot use function `remove` while the list is in use"
                REMOVE_LIST_DOC_);
        goto fail1;
    }

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

    if (ti_closure_try_lock(closure, e))
        goto fail1;

    if (ti_scope_local_from_closure(query->scope, closure, e))
        goto fail2;

    for (vec_each(varr->vec, ti_val_t, v), ++idx)
    {
        if (ti_scope_polute_val(query->scope, v, idx))
        {
            ex_set_mem(e);
            goto fail2;
        }

        if (ti_do_optscope(query, ti_closure_scope_nd(closure), e))
            goto fail2;

        found = ti_val_as_bool(query->rval);
        ti_val_drop(query->rval);

        if (found)
        {
            query->rval = v;  /* we can move the reference here */
            (void) vec_remove(varr->vec, idx);

            if (is_attached)
            {
                ti_task_t * task;
                assert (query->scope->thing);
                assert (query->scope->name);
                task = ti_task_get_task(query->ev, query->scope->thing, e);
                if (!task)
                    goto fail2;

                if (ti_task_add_splice(
                        task,
                        query->scope->name,
                        NULL,
                        idx,
                        1,
                        0))
                    ex_set_mem(e);
            }

            goto done;
        }
        query->rval = NULL;
    }

    assert (query->rval == NULL);
    if (nargs == 2)
    {
        /* lazy evaluation of the alternative value */
        (void) ti_do_scope(query, nd->children->next->next->node, e);
    }
    else
    {
        query->rval = (ti_val_t *) ti_nil_get();
    }

done:
fail2:
    ti_closure_unlock(closure);

fail1:
    ti_val_drop((ti_val_t *) closure);
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

    if (ti_closure_try_lock(closure, e))
        goto fail1;

    if (ti_scope_local_from_closure(query->scope, closure, e))
        goto fail2;

    for (vec_each(vec, ti_thing_t, t))
    {
        if (ti_scope_polute_val(query->scope, (ti_val_t *) t, t->id))
        {
            ex_set_mem(e);
            goto fail2;
        }

        if (ti_do_optscope(query, ti_closure_scope_nd(closure), e))
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
    ti_closure_unlock(closure);

fail1:
    ti_val_drop((ti_val_t *) closure);
    return e->nr;
}

static void do__f_remove_set(
        ti_vset_t * vset,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    vec_t * removed = NULL;
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool is_attached = ti_scope_is_attached(query->scope);

    assert (ti_val_is_set((ti_val_t *) vset));

    if (nargs < 1)
    {
        ex_set(e, EX_BAD_DATA,
                "function `remove` requires at least 1 argument but 0 "
                "were given"REMOVE_SET_DOC_);
        return;
    }

    if (is_attached &&
        ti_scope_in_use_val(query->scope->prev, (ti_val_t *) vset))
    {
        ex_set(e, EX_BAD_DATA,
                "cannot use function `remove` while the set is in use"
                REMOVE_SET_DOC_);
        return;
    }

    if (ti_do_scope(query, nd->children->node, e))
        return;

    if (ti_val_is_closure(query->rval))
    {
        removed = vec_new(1);
        if (!removed)
        {
            ex_set_mem(e);
            return;
        }

        if (do__f_remove_set_from_closure(&removed, vset, query, nargs, e))
            goto failed;

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
            return;
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

                goto failed;
            }

            if (ti_vset_pop(vset, (ti_thing_t *) query->rval))
                VEC_push(removed, query->rval);

            ti_val_drop(query->rval);
            query->rval = NULL;

            if (!child->next || !(child = child->next->next))
                break;

            if (ti_do_scope(query, child->node, e))
                goto failed;
        }
    }

    if (removed->n && is_attached)
    {
        ti_task_t * task = ti_task_get_task(query->ev, query->scope->thing, e);
        if (!task)
            goto failed;

        if (ti_task_add_remove(
                task,
                query->scope->name,
                removed))
        {
            ex_set_mem(e);
            goto failed;
        }
    }

    assert (query->rval == NULL);
    query->rval = (ti_val_t *) ti_varr_from_vec(removed);
    if (query->rval)
        return;

    ex_set_mem(e);

failed:
    while (removed->n)
        /* if the `thing` is already in the set, then the set still owns the
         * thing, else the thing was owned by the `removed` vector so in
         * neither case we have o adjust the reference counter */
        if (ti_vset_add(vset, vec_pop(removed)) == IMAP_ERR_ALLOC)
            ex_set_mem(e);
    free(removed);
}

static int do__f_remove(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * val = ti_query_val_pop(query);

    if (ti_val_is_list(val))
        do__f_remove_list((ti_varr_t *) val, query, nd, e);
    else if (ti_val_is_set(val))
        do__f_remove_set((ti_vset_t *) val, query, nd, e);
    else
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `remove`"
                REMOVE_LIST_DOC_ REMOVE_SET_DOC_,
                ti_val_str(val));

    ti_val_drop(val);
    return e->nr;
}
