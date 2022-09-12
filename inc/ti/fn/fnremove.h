#include <ti/fn/fn.h>

static int do__f_remove_list(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    intptr_t idx;
    int64_t limit;
    ti_closure_t * closure;
    ti_varr_t * varr;
    vec_t * vec;

    if (fn_nargs_range("remove", DOC_LIST_REMOVE, 1, 2, nargs, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    varr = (ti_varr_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e) ||
        fn_arg_closure("remove", DOC_LIST_REMOVE, 1, query->rval, e))
        goto fail1;

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (nargs == 2)
    {
        if (ti_do_statement(query, nd->children->next->next, e) ||
            fn_arg_int("remove", DOC_LIST_REMOVE, 2, query->rval, e))
            goto fail2;
        limit = VINT(query->rval);
        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }
    else
        limit = LLONG_MAX;

    if (    ti_closure_try_wse(closure, query, e) ||
            ti_closure_inc(closure, query, e))
        goto fail2;

    vec = vec_new(1);
    if (!vec)
    {
        ex_set_mem(e);
        goto fail3;
    }

    if (limit >= 0)
    {
        idx = 0;
        for (vec_each(varr->vec, ti_val_t, v), ++idx)
        {
            if (vec->n == limit)
                break;

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
    }
    else
    {
        idx = varr->vec->n;
        limit = limit == LLONG_MIN ? LLONG_MAX : llabs(limit);

        for (vec_each_rev(varr->vec, ti_val_t, v))
        {
            if (vec->n == limit)
                break;

            if (ti_closure_vars_val_idx(closure, v, --idx))
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
        vec_reverse(vec);
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

    for (vec_each_rev(vec, void, pos))
        *v__ = vec_remove(varr->vec, (uintptr_t) pos);

    query->rval = (ti_val_t *) ti_varr_from_vec_unsafe(vec);
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
    int64_t limit;
} remove__walk_set_t;

static int remove__walk(ti_thing_t * t, remove__walk_set_t * w)
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

    return (*w->removed)->n == w->limit;
}

static int do__f_remove_set_from_closure(
        vec_t ** removed,
        ti_vset_t * vset,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_closure_t * closure = (ti_closure_t *) query->rval;
    int64_t limit;
    query->rval = NULL;

    if (nargs == 2)
    {
        if (ti_do_statement(query, nd->children->next->next, e) ||
            fn_arg_int("remove", DOC_SET_REMOVE, 2, query->rval, e))
            goto fail;

        limit = VINT(query->rval);
        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }
    else if (nargs > 2)
    {
        ex_set(e, EX_NUM_ARGUMENTS,
                "function `remove` takes at most 2 arguments when using a `"
                TI_VAL_CLOSURE_S"` but %d were given"DOC_SET_REMOVE,
                nargs);
        goto fail;
    }
    else
        limit = LLONG_MAX;

    if (    ti_closure_try_wse(closure, query, e) ||
            ti_closure_inc(closure, query, e))
        goto fail;

    remove__walk_set_t w = {
            .e = e,
            .closure = closure,
            .query = query,
            .removed = removed,
            .limit = limit,
    };

    if (ti_closure_wse(closure) && ti_vset_has_relation(vset))
    {
        if (limit && imap_walk_cp(
                vset->imap,
                (imap_cb) remove__walk,
                &w,
                (imap_destroy_cb) ti_val_unsafe_drop) && !e->nr)
            ex_set_mem(e);
    }
    else if (limit)
        (void) imap_walk(vset->imap, (imap_cb) remove__walk, &w);

    ti_closure_dec(closure, query);

fail:
    ti_val_unsafe_drop((ti_val_t *) closure);
    return e->nr;
}

static int do__f_remove_set(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    vec_t * removed = NULL;
    const int nargs = fn_get_nargs(nd);
    ti_vset_t * vset;

    if (fn_nargs_min("remove", DOC_SET_REMOVE, 1, nargs, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    vset = (ti_vset_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e))
        goto fail1;

    if (ti_val_is_closure(query->rval))
    {
        removed = vec_new(1);
        if (!removed)
        {
            ex_set_mem(e);
            goto fail1;
        }

        if (do__f_remove_set_from_closure(&removed, vset, query, nd, e))
            goto fail2;

        assert (query->rval == NULL);

        for (vec_each(removed, ti_thing_t, t))
            (void) ti_vset_pop(vset, t);
    }
    else
    {
        size_t narg;
        cleri_node_t * child = nd->children;

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

            if (ti_do_statement(query, child, e))
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

        if (ti_vset_has_relation(vset))
        {
            ti_field_t * field = vset->key_;
            ti_field_t * ofield = field->condition.rel->field;
            for (vec_each(removed, ti_thing_t, thing))
                ofield->condition.rel->del_cb(
                        ofield,
                        thing,
                        vset->parent);
        }
    }

    assert (query->rval == NULL);
    query->rval = (ti_val_t *) ti_varr_from_vec_unsafe(removed);
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

typedef struct
{
    ex_t * e;
    ti_closure_t * closure;
    ti_query_t * query;
    vec_t ** removed;
    ti_thing_t * thing;
    int64_t limit;
    size_t * alloc_sz;
} remove__walk_i_t;

static int remove__walk_i(ti_item_t * item, remove__walk_i_t * w)
{
    if (ti_closure_vars_item(w->closure, item, w->e) ||
        ti_closure_do_statement(w->closure, w->query, w->e))
        return -1;

    if (ti_val_as_bool(w->query->rval))
    {
        if (ti_val_tlocked(
                item->val,
                w->thing,
                (ti_name_t *) item->key,
                w->e))
            return -1;

        if (vec_push(w->removed, item))
        {
            ex_set_mem(w->e);
            return -1;
        }
        /* add to size for later allocation */
        *w->alloc_sz += item->key->n;
    }

    ti_val_unsafe_drop(w->query->rval);
    w->query->rval = NULL;
    return (*w->removed)->n == w->limit;
}

static int do__f_remove_thing(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    int64_t limit;
    ti_closure_t * closure;
    ti_thing_t * thing;
    vec_t * vec;
    size_t alloc_sz = 0;

    if (fn_nargs_range("remove", DOC_THING_REMOVE, 1, 2, nargs, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e) ||
        fn_arg_closure("remove", DOC_THING_REMOVE, 1, query->rval, e))
        goto fail1;

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (nargs == 2)
    {
        if (ti_do_statement(query, nd->children->next->next, e) ||
            fn_arg_int("remove", DOC_THING_REMOVE, 2, query->rval, e))
            goto fail2;
        limit = VINT(query->rval);
        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }
    else
        limit = LLONG_MAX;

    if (    ti_closure_try_wse(closure, query, e) ||
            ti_closure_inc(closure, query, e))
        goto fail2;

    vec = vec_new(1);
    if (!vec)
    {
        ex_set_mem(e);
        goto fail3;
    }

    if (ti_thing_is_dict(thing))
    {
        remove__walk_i_t w = {
                .e = e,
                .closure = closure,
                .query = query,
                .removed = &vec,
                .thing = thing,
                .limit = limit,
                .alloc_sz = &alloc_sz,
        };

        if (limit && smap_values(
                thing->items.smap,
                (smap_val_cb) remove__walk_i,
                &w) < 0)
            goto fail3;
    }
    else
    {
        for (vec_each(thing->items.vec, ti_prop_t, prop))
        {
            if (vec->n == limit)
                break;

            if (ti_closure_vars_prop(closure, prop, e) ||
                ti_closure_do_statement(closure, query, e))
                goto fail3;

            if (ti_val_as_bool(query->rval))
            {
                if (ti_val_tlocked(prop->val, thing, prop->name, e))
                    goto fail3;

                if (vec_push(&vec, prop))
                {
                    ex_set_mem(e);
                    goto fail3;
                }
                alloc_sz += prop->name->n;
            }

            ti_val_unsafe_drop(query->rval);
            query->rval = NULL;
        }
    }


    if (vec->n && thing->id)
    {
        ti_task_t * task;
        task = ti_task_get_task(query->change, thing);
        if (!task || ti_task_add_thing_remove(task, vec, alloc_sz))
        {
            ex_set_mem(e);
            goto fail3;
        }
    }

    for (vec_each(vec, ti_prop_t, prop))
    {
        *v__ = (void *) prop->val;
        ti_incref(prop->val);
        ti_thing_o_del(thing, prop->name->str, prop->name->n);
    }

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
    ti_val_unlock((ti_val_t *) thing, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) thing);
    return e->nr;
}

static inline int do__f_remove(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    return ti_val_is_list(query->rval)
            ? do__f_remove_list(query, nd, e)
            : ti_val_is_set(query->rval)
            ? do__f_remove_set(query, nd, e)
            : ti_val_is_object(query->rval)
            ? do__f_remove_thing(query, nd, e)
            : fn_call_try("remove", query, nd, e);
}
