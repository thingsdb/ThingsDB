#include <ti/fn/fn.h>

static int do__f_some(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool some = false;
    int64_t idx  = 0;
    ti_varr_t * varr;
    ti_closure_t * closure;

    if (!ti_val_is_array(query->rval))
        return fn_call_try("some", query, nd, e);

    if (fn_nargs("some", DOC_LIST_SOME, 1, nargs, e))
        return e->nr;

    varr = (ti_varr_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e))
        goto fail0;

    if (!ti_val_is_closure(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `some` expects argument 1 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead"
                DOC_LIST_SOME,
                ti_val_str(query->rval));
        goto fail0;
    }

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (    ti_closure_try_wse(closure, query, e) ||
            ti_closure_try_lock_and_use(closure, query, e))
        goto fail1;

    for (vec_each(varr->vec, ti_val_t, v), ++idx)
    {
        if (ti_closure_vars_val_idx(closure, v, idx))
        {
            ex_set_mem(e);
            goto fail2;
        }

        if (ti_closure_do_statement(closure, query, e))
            goto fail2;

        some = ti_val_as_bool(query->rval);
        ti_val_drop(query->rval);

        if (some)
            break;

        query->rval = NULL;
    }

    query->rval = (ti_val_t *) ti_vbool_get(some);
fail2:
    ti_closure_unlock_use(closure, query);
fail1:
    ti_val_drop((ti_val_t *) closure);
fail0:
    ti_val_drop((ti_val_t *) varr);
    return e->nr;
}
