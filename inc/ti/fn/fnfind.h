#include <ti/fn/fn.h>

#define FIND_DOC_ TI_SEE_DOC("#find")

static int do__f_find(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    int64_t idx = 0;
    ti_closure_t * closure;
    ti_val_t * iterval;
    int lock_was_set;

    if (fn_not_chained("find", query, e))
        return e->nr;

    if (    !ti_val_is_array(query->rval) &&
            !ti_val_is_set(query->rval))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no function `find`"FIND_DOC_,
                ti_val_str(query->rval));
        return e->nr;
    }

    if (nargs < 1)
    {
        ex_set(e, EX_NUM_ARGUMENTS,
                "function `find` requires at least 1 argument but 0 "
                "were given"FIND_DOC_);
        return e->nr;
    }

    if (nargs > 2)
    {
        ex_set(e, EX_NUM_ARGUMENTS,
                "function `find` takes at most 2 arguments but %d "
                "were given"FIND_DOC_, nargs);
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
                "function `find` expects argument 1 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead"FIND_DOC_,
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
    case TI_VAL_ARR:
        for (vec_each(((ti_varr_t *) iterval)->vec, ti_val_t, v), ++idx)
        {
            _Bool found;

            if (ti_closure_vars_val_idx(closure, v, idx))
            {
                ex_set_mem(e);
                goto fail2;
            }

            if (ti_closure_do_statement(closure, query, e))
                goto fail2;

            found = ti_val_as_bool(query->rval);
            ti_val_drop(query->rval);

            if (found)
            {
                query->rval = v;
                ti_incref(v);
                goto done;
            }

            query->rval = NULL;
        }
        break;
    case TI_VAL_SET:
    {
        vec_t * vec = imap_vec(((ti_vset_t *) iterval)->imap);
        if (!vec)
        {
            ex_set_mem(e);
            goto fail2;
        }

        for (vec_each(vec, ti_thing_t, t))
        {
            _Bool found;

            if (ti_closure_vars_val_idx(closure, (ti_val_t *) t, t->id))
            {
                ex_set_mem(e);
                goto fail2;
            }

            if (ti_closure_do_statement(closure, query, e))
                goto fail2;

            found = ti_val_as_bool(query->rval);
            ti_val_drop(query->rval);

            if (found)
            {
                query->rval = (ti_val_t *) t;
                ti_incref(t);
                goto done;
            }

            query->rval = NULL;
        }
        break;
    }
    }

    assert (query->rval == NULL);
    if (nargs == 2)
    {
        /* lazy evaluation of the alternative value */
        (void) ti_do_statement(query, nd->children->next->next->node, e);
    }
    else
    {
        query->rval = (ti_val_t *) ti_nil_get();
    }

done:
fail2:
    ti_closure_unlock_use(closure, query);

fail1:
    ti_val_drop((ti_val_t *) closure);

fail0:
    ti_val_unlock(iterval, lock_was_set);
    ti_val_drop(iterval);
    return e->nr;
}
