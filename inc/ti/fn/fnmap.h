#include <ti/fn/fn.h>

#define MAP_DOC_ TI_SEE_DOC("#map")

static int do__f_map(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    size_t n;
    ti_varr_t * retvarr;
    ti_closure_t * closure;
    ti_val_t * iterval;
    int lock_was_set;

    if (fn_not_chained("map", query, e))
        return e->nr;

    if (    !ti_val_is_arr(query->rval) &&
            !ti_val_is_set(query->rval) &&
            !ti_val_is_thing(query->rval))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no function `map`"MAP_DOC_,
                ti_val_str(query->rval));
        return e->nr;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_NUM_ARGUMENTS,
                "function `map` takes 1 argument but %d were given"MAP_DOC_,
                nargs);
        return e->nr;
    }

    lock_was_set = ti_val_ensure_lock(query->rval);
    iterval = query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e))
        goto fail0;

    if (!ti_val_is_closure(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `map` expects argument 1 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead"MAP_DOC_,
                ti_val_str(query->rval));
        goto fail0;
    }

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    n = ti_val_get_len(iterval);

    if (    ti_closure_try_wse(closure, query, e) ||
            ti_closure_try_lock_and_use(closure, query, e))
        goto fail1;

    retvarr = ti_varr_create(n);
    if (!retvarr)
        goto fail2;

    switch (iterval->tp)
    {
    case TI_VAL_THING:
    {
        ti_thing_t * thing = (ti_thing_t *) iterval;

        if (ti_thing_is_object(thing))
        {
            for (vec_each(thing->items, ti_prop_t, p))
            {
                if (ti_closure_vars_prop(closure, p, e))
                    goto fail2;

                if (ti_closure_do_statement(closure, query, e))
                    goto fail2;

                if (ti_varr_append(retvarr, (void **) &query->rval, e))
                    goto fail2;

                query->rval = NULL;
            }
        }
        else
        {
            ti_name_t * name;
            ti_val_t * val;
            for (thing_each(thing, name, val))
            {
                if (ti_closure_vars_nameval(closure, name, val, e))
                    goto fail2;

                if (ti_closure_do_statement(closure, query, e))
                    goto fail2;

                if (ti_varr_append(retvarr, (void **) &query->rval, e))
                    goto fail2;

                query->rval = NULL;
            }
        }
        break;
    }
    case TI_VAL_ARR:
    {
        int64_t idx = 0;
        for (vec_each(((ti_varr_t *) iterval)->vec, ti_val_t, v), ++idx)
        {
            if (ti_closure_vars_val_idx(closure, v, idx))
                goto fail2;

            if (ti_closure_do_statement(closure, query, e))
                goto fail2;

            if (ti_varr_append(retvarr, (void **) &query->rval, e))
                goto fail2;

            query->rval = NULL;
        }
        break;
    }
    case TI_VAL_SET:
    {
        vec_t * vec = imap_vec(((ti_vset_t *) iterval)->imap);
        if (!vec)
            goto fail2;
        for (vec_each(vec, ti_thing_t, t))
        {
            if (ti_closure_vars_val_idx(closure, (ti_val_t *) t, t->id))
                goto fail2;

            if (ti_closure_do_statement(closure, query, e))
                goto fail2;

            if (ti_varr_append(retvarr, (void **) &query->rval, e))
                goto fail2;

            query->rval = NULL;
        }
    }
    }

    assert (query->rval == NULL);
    assert (retvarr->vec->n == n);
    query->rval = (ti_val_t *) retvarr;

    goto done;

fail2:
    if (!e->nr)
        ex_set_mem(e);
    ti_val_drop((ti_val_t *) retvarr);

done:
    ti_closure_unlock_use(closure, query);

fail1:
    ti_val_drop((ti_val_t *) closure);

fail0:
    ti_val_unlock(iterval, lock_was_set);
    ti_val_drop(iterval);
    return e->nr;
}
