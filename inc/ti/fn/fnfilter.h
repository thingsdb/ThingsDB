#include <ti/fn/fn.h>

static int do__f_filter(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const char * doc;
    const int nargs = langdef_nd_n_function_params(nd);
    ti_val_t * retval = NULL;
    ti_closure_t * closure;
    ti_val_t * iterval;
    int lock_was_set;

    if (fn_not_chained("filter", query, e))
        return e->nr;

    doc = doc_filter(query->rval);
    if (!doc)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no function `filter`",
                ti_val_str(query->rval));
        return e->nr;
    }

    if (fn_nargs("filter", doc, 1, nargs, e))
        return e->nr;

    lock_was_set = ti_val_ensure_lock(query->rval);
    iterval = query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e))
        goto fail0;

    if (!ti_val_is_closure(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `filter` expects argument 1 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead%s",
                ti_val_str(query->rval), doc);
        goto fail0;
    }

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (    ti_closure_try_wse(closure, query, e) ||
            ti_closure_try_lock_and_use(closure, query, e))
        goto fail1;

    switch (iterval->tp)
    {
    case TI_VAL_THING:
    {
        ti_thing_t * thing, * t = (ti_thing_t *) iterval;

        if (ti_quota_things(
                query->collection->quota,
                query->collection->things->n, e))
            goto fail2;

        thing = ti_thing_o_create(0, t->items->n, query->collection);
        if (!thing)
            goto fail2;

        retval = (ti_val_t *) thing;

        if (ti_thing_is_object(t))
        {
            for (vec_each(t->items, ti_prop_t, p))
            {
                if (ti_closure_vars_prop(closure, p, e))
                    goto fail2;

                if (ti_closure_do_statement(closure, query, e))
                    goto fail2;

                if (ti_val_as_bool(query->rval))
                {
                    if (    ti_val_make_assignable(&p->val, e) ||
                            !ti_thing_o_prop_add(thing, p->name, p->val))
                        goto fail2;
                    ti_incref(p->name);
                    ti_incref(p->val);
                }

                ti_val_drop(query->rval);
                query->rval = NULL;
            }
        }
        else
        {
            ti_name_t * name;
            ti_val_t * val;
            for (thing_each(t, name, val))
            {
                if (ti_closure_vars_nameval(closure, name, val, e))
                    goto fail2;

                if (ti_closure_do_statement(closure, query, e))
                    goto fail2;

                if (ti_val_as_bool(query->rval))
                {
                    if (    ti_val_make_assignable(&val, e) ||
                            !ti_thing_o_prop_add(thing, name, val))
                        goto fail2;
                    ti_incref(name);
                    ti_incref(val);
                }

                ti_val_drop(query->rval);
                query->rval = NULL;
            }

        }
        break;
    }
    case TI_VAL_ARR:
    {
        int64_t idx = 0;
        ti_varr_t * varr = ti_varr_create(((ti_varr_t *) iterval)->vec->n);
        if (!varr)
            goto fail2;

        retval = (ti_val_t *) varr;

        for (vec_each(((ti_varr_t *) iterval)->vec, ti_val_t, v), ++idx)
        {
            if (ti_closure_vars_val_idx(closure, v, idx))
                goto fail2;

            if (ti_closure_do_statement(closure, query, e))
                goto fail2;

            if (ti_val_as_bool(query->rval))
            {
                ti_incref(v);
                VEC_push(varr->vec, v);
            }

            ti_val_drop(query->rval);
            query->rval = NULL;

        }
        (void) vec_shrink(&varr->vec);
        break;
    }
    case TI_VAL_SET:
    {
        vec_t * vec = imap_vec(((ti_vset_t *) iterval)->imap);
        ti_vset_t * vset = ti_vset_create();
        if (!vset || !vec)
            goto fail2;

        retval = (ti_val_t *) vset;

        for (vec_each(vec, ti_thing_t, t))
        {
            if (ti_closure_vars_val_idx(closure, (ti_val_t *) t, t->id))
                goto fail2;

            if (ti_closure_do_statement(closure, query, e))
                goto fail2;

            if (ti_val_as_bool(query->rval))
            {
                if (ti_vset_add(vset, t))
                    goto fail2;
                ti_incref(t);
            }

            ti_val_drop(query->rval);
            query->rval = NULL;
        }
    }
    }

    assert (query->rval == NULL);
    query->rval = retval;

    goto done;

fail2:
    if (!e->nr)
        ex_set_mem(e);
    ti_val_drop(retval);

done:
    ti_closure_unlock_use(closure, query);

fail1:
    ti_val_drop((ti_val_t *) closure);

fail0:
    ti_val_unlock(iterval, lock_was_set);
    ti_val_drop(iterval);
    return e->nr;
}
