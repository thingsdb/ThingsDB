#include <ti/fn/fn.h>

#define FIND_DOC_ TI_SEE_DOC("#find")

static int do__f_find(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    const int nargs = langdef_nd_n_function_params(nd);
    int64_t idx = 0;
    ti_closure_t * closure = NULL;
    ti_val_t * val = ti_query_val_pop(query);

    if (!ti_val_is_array(val) && !ti_val_is_set(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `find`"FIND_DOC_,
                ti_val_str(val));
        goto fail1;
    }

    if (nargs < 1)
    {
        ex_set(e, EX_BAD_DATA,
                "function `find` requires at least 1 argument but 0 "
                "were given"FIND_DOC_);
        goto fail1;
    }

    if (nargs > 2)
    {
        ex_set(e, EX_BAD_DATA,
                "function `find` takes at most 2 arguments but %d "
                "were given"FIND_DOC_, nargs);
        goto fail1;
    }

    if (ti_do_scope(query, nd->children->node, e))
        goto fail1;

    if (!ti_val_is_closure(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `find` expects argument 1 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead"FIND_DOC_,
                ti_val_str(query->rval));
        goto fail1;
    }

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (ti_closure_try_lock(closure, e))
        goto fail1;

    if (ti_scope_local_from_closure(query->scope, closure, e))
        goto fail2;

    switch (val->tp)
    {
    case TI_VAL_ARR:
        for (vec_each(((ti_varr_t *) val)->vec, ti_val_t, v), ++idx)
        {
            _Bool found;

            if (ti_scope_polute_val(query->scope, v, idx))
            {
                ex_set_alloc(e);
                goto fail2;
            }

            if (ti_do_optscope(query, ti_closure_scope_nd(closure), e))
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
        vec_t * vec = imap_vec(((ti_vset_t *) val)->imap);
        if (!vec)
        {
            ex_set_alloc(e);
            goto fail2;
        }

        for (vec_each(vec, ti_thing_t, t))
        {
            _Bool found;

            if (ti_scope_polute_val(query->scope, (ti_val_t *) t, t->id))
            {
                ex_set_alloc(e);
                goto fail2;
            }

            if (ti_do_optscope(query, ti_closure_scope_nd(closure), e))
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
    ti_val_drop(val);

    return e->nr;
}
