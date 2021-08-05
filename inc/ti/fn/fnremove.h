#include <ti/fn/fn.h>

static int do__f_remove_list(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    uintptr_t idx = 0;
    ti_closure_t * closure;
    ti_varr_t * varr;
    vec_t * vec;

    if (fn_nargs("remove", DOC_LIST_REMOVE, 1, nargs, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    varr = (ti_varr_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e) ||
        fn_arg_closure("remove", DOC_LIST_REMOVE, 1, query->rval, e))
        goto fail1;

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (    ti_closure_try_wse(closure, query, e) ||
            ti_closure_inc(closure, query, e))
        goto fail2;

    vec = vec_new(1);
    if (!vec)
    {
        ex_set_mem(e);
        goto fail3;
    }

    for (vec_each(varr->vec, ti_val_t, v), ++idx)
    {
        if (ti_closure_vars_val_idx(closure, v, idx))
        {
            ex_set_mem(e);
            goto fail3;
        }

        if (ti_closure_do_statement(closure, query, e))
            goto fail3;

        if (ti_val_as_bool(query->rval) && vec_push(&vec, (void *) idx))
        {
            ex_set_mem(e);
            goto fail3;
        }

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }


    if (vec->n && varr->parent && varr->parent->id)
    {
        ti_task_t * task;
        task = ti_task_get_task(query->change, varr->parent);
        if (!task || ti_task_add_arr_remove(task, ti_varr_key(varr), vec))
        {
            ex_set_mem(e);
            goto fail3;
        }
    }

    idx = vec->n;
    for (vec_each_rev(vec, void, pos))
        VEC_set(vec, vec_remove(varr->vec, (uintptr_t) pos), --idx);

    query->rval = (ti_val_t *) ti_varr_from_vec(vec);
    if (query->rval)
        vec = NULL;
    else
        ex_set_mem(e);

fail3:
    free(vec);
    ti_closure_dec(closure, query);

fail2:
    ti_val_unsafe_drop((ti_val_t *) closure);

fail1:
    ti_val_unlock((ti_val_t *) varr, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) varr);
    return e->nr;
}

typedef struct
{
    ex_t * e;
    ti_closure_t * closure;
    ti_query_t * query;
    vec_t ** removed;
} remove__walk_t;

static int remove__walk(ti_thing_t * t, remove__walk_t * w)
{
    if (ti_closure_vars_vset(w->closure, t))
    {
        ex_set_mem(w->e);
        return -1;
    }

    if (ti_closure_do_statement(w->closure, w->query, w->e))
        return -1;

    if (ti_val_as_bool(w->query->rval) && vec_push(w->removed, t))
    {
        ex_set_mem(w->e);
        return -1;
    }

    ti_val_unsafe_drop(w->query->rval);
    w->query->rval = NULL;
    return 0;
}

static int do__f_remove_set_from_closure(
        vec_t ** removed,
        ti_vset_t * vset,
        ti_query_t * query,
        const int nargs,
        ex_t * e)
{
    ti_closure_t * closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    /* do not use the usual arguments check since we want a special message */
    if (nargs > 1)
    {
        ex_set(e, EX_NUM_ARGUMENTS,
                "function `remove` takes at most 1 argument when using a `"
                TI_VAL_CLOSURE_S"` but %d were given"DOC_SET_REMOVE,
                nargs);
        goto fail;
    }

    if (    ti_closure_try_wse(closure, query, e) ||
            ti_closure_inc(closure, query, e))
        goto fail;

    remove__walk_t w = {
            .e = e,
            .closure = closure,
            .query = query,
            .removed = removed,
    };

    if (ti_vset_has_relation(vset))
    {
        if (imap_walk_cp(
                vset->imap,
                (imap_cb) remove__walk,
                &w,
                (imap_destroy_cb) ti_val_unsafe_drop) && !e->nr)
            ex_set_mem(e);
    }
    else
        (void) imap_walk(vset->imap, (imap_cb) remove__walk, &w);

    ti_closure_dec(closure, query);

fail:
    ti_val_unsafe_drop((ti_val_t *) closure);
    return e->nr;
}

static int do__f_remove_set(
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    vec_t * removed = NULL;
    const int nargs = fn_get_nargs(nd);
    ti_vset_t * vset;

    if (fn_nargs_min("remove", DOC_SET_REMOVE, 1, nargs, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    vset = (ti_vset_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e))
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
            (void) ti_vset_pop(vset, t);
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
                ex_set(e, EX_TYPE_ERROR,
                        narg == 1
                        ?
                        "function `remove` expects argument %d to be of type "
                        "`"TI_VAL_CLOSURE_S"` or type `"TI_VAL_THING_S"` "
                        "but got type `%s` instead"DOC_SET_REMOVE
                        :
                        "function `remove` expects argument %d to be "
                        "of type `"TI_VAL_THING_S"` "
                        "but got type `%s` instead"DOC_SET_REMOVE,
                        narg, ti_val_str(query->rval));

                goto fail2;
            }

            if (ti_vset_pop(vset, (ti_thing_t *) query->rval))
                VEC_push(removed, query->rval);

            ti_val_unsafe_drop(query->rval);
            query->rval = NULL;

            if (!child->next || !(child = child->next->next))
                break;

            if (ti_do_statement(query, child->node, e))
                goto fail2;
        }
    }

    if (removed->n && vset->parent)
    {
        if (vset->parent->id)
        {
            ti_task_t * task = ti_task_get_task(query->change, vset->parent);
            if (!task || ti_task_add_set_remove(
                    task,
                    ti_vset_key(vset),
                    removed))
            {
                ex_set_mem(e);
                goto fail2;
            }
        }

        if (ti_thing_is_instance(vset->parent))
        {
            ti_field_t * field = vset->key_;
            if (field->condition.rel)
            {
                ti_field_t * ofield = field->condition.rel->field;
                for (vec_each(removed, ti_thing_t, thing))
                    ofield->condition.rel->del_cb(
                            ofield,
                            thing,
                            vset->parent);
            }
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
        if (ti_vset_add(vset, VEC_pop(removed)) == IMAP_ERR_ALLOC)
            ex_set_mem(e);
    free(removed);

fail1:
done:
    ti_val_unlock((ti_val_t *) vset, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) vset);
    return e->nr;
}

static inline int do__f_remove(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    return ti_val_is_list(query->rval)
            ? do__f_remove_list(query, nd, e)
            : ti_val_is_set(query->rval)
            ? do__f_remove_set(query, nd, e)
            : fn_call_try("remove", query, nd, e);
}
