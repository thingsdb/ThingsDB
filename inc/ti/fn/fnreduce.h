#include <ti/fn/fn.h>

static int do__f_reduce(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_closure_t * closure;
    ti_val_t * lockval;
    vec_t * vec;
    int lock_was_set;

    if (!ti_val_is_array(query->rval))
        return fn_call_try("reduce", query, nd, e);

    if (fn_nargs_range("reduce", DOC_LIST_REDUCE, 1, 2, nargs, e))
        return e->nr;

    lock_was_set = ti_val_ensure_lock(query->rval);
    lockval = query->rval;
    query->rval = NULL;

    vec = ((ti_varr_t *) lockval)->vec;

    if (ti_do_statement(query, nd->children->node, e))
        goto fail0;

    if (!ti_val_is_closure(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `reduce` expects argument 1 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead"
                DOC_LIST_REDUCE,
                ti_val_str(query->rval));
        goto fail0;
    }

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (    ti_closure_try_wse(closure, query, e) ||
            ti_closure_try_lock_and_use(closure, query, e))
        goto fail1;

    if (nargs == 2)
    {
        if (ti_do_statement(query, nd->children->next->next->node, e))
            goto fail2;
    }
    else if (vec->n == 0)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "reduce on empty list with no initial value set");
        goto fail2;
    }
    else
    {
        query->rval = vec_get(vec, 0);
        ti_incref(query->rval);
    }

    for (size_t idx = nargs & 1, m = vec->n; idx < m; ++idx)
    {
        ti_val_t * v;
        ti_prop_t * prop;
        switch(closure->vars->n)
        {
        default:
        case 3:
            prop = vec_get(closure->vars, 2);
            ti_val_drop(prop->val);
            prop->val = (ti_val_t *) ti_vint_create(idx);
            if (!prop->val)
            {
                ex_set_mem(e);
                goto fail2;
            }
            /* fall through */
        case 2:
            prop = vec_get(closure->vars, 1);
            v = vec_get(vec, idx);
            ti_incref(v);
            ti_val_drop(prop->val);
            prop->val = v;
            /* fall through */
        case 1:
            prop = vec_get(closure->vars, 0);
            ti_val_drop(prop->val);
            prop->val = query->rval;
            query->rval = NULL;
            break;
        case 0:
            ti_val_drop(query->rval);
            query->rval = NULL;
            break;
        }
        if (ti_closure_do_statement(closure, query, e))
            goto fail2;
    }

fail2:
    ti_closure_unlock_use(closure, query);

fail1:
    ti_val_drop((ti_val_t *) closure);

fail0:
    ti_val_unlock(lockval, lock_was_set);
    ti_val_drop(lockval);
    return e->nr;
}
