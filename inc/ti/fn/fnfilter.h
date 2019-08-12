#include <ti/fn/fn.h>

#define FILTER_DOC_ TI_SEE_DOC("#filter")

static int do__f_filter(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * retval = NULL;
    ti_closure_t * closure;
    ti_val_t * iterval;
    int lock_was_set;

    if (    !ti_val_is_arr(query->rval) &&
            !ti_val_is_set(query->rval) &&
            !ti_val_is_thing(query->rval))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `filter`"FILTER_DOC_,
                ti_val_str(query->rval));
        return e->nr;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `filter` takes 1 argument but %d were given"
                FILTER_DOC_, nargs);
        return e->nr;
    }

    lock_was_set = ti_val_ensure_lock(query->rval);
    iterval = query->rval;
    query->rval = NULL;

    if (ti_do_scope(query, nd->children->node, e))
        goto fail0;

    if (!ti_val_is_closure(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `filter` expects argument 1 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead"FILTER_DOC_,
                ti_val_str(query->rval));
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
        ti_thing_t * thing;

        if (ti_quota_things(query->target->quota, query->target->things->n, e))
            goto fail2;

        thing = ti_thing_create(0, query->target->things);
        if (!thing)
            goto fail2;

        retval = (ti_val_t *) thing;

        for (vec_each(((ti_thing_t *) iterval)->props, ti_prop_t, p))
        {
            if (ti_closure_vars_prop(closure, p, e))
                goto fail2;

            if (ti_do_scope(query, ti_closure_scope(closure), e))
                goto fail2;

            if (ti_val_as_bool(query->rval))
            {
                if (    ti_val_make_assignable(&p->val, e) ||
                        !ti_thing_prop_add(thing, p->name, p->val))
                    goto fail2;
                ti_incref(p->name);
                ti_incref(p->val);
            }

            ti_val_drop(query->rval);
            query->rval = NULL;
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

            if (ti_do_scope(query, ti_closure_scope(closure), e))
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

            if (ti_do_scope(query, ti_closure_scope(closure), e))
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
