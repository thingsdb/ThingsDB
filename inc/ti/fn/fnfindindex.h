#include <ti/fn/fn.h>

static int do__f_findindex(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    size_t idx = 0;
    ti_varr_t * varr;
    ti_closure_t * closure;
    int lock_was_set;

    if (!ti_val_is_array(query->rval))
        return fn_call_try("findindex", query, nd, e);

    if (fn_nargs("findindex", DOC_LIST_FINDINDEX, 1, nargs, e))
        return e->nr;

    lock_was_set = ti_val_ensure_lock(query->rval);
    varr = (ti_varr_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e) ||
        fn_arg_closure("findindex", DOC_LIST_FINDINDEX, 1, query->rval, e))
        goto fail0;

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (    ti_closure_try_wse(closure, query, e) ||
            ti_closure_inc(closure, query, e))
        goto fail1;


    for (vec_each(varr->vec, ti_val_t, v), ++idx)
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
            query->rval = (ti_val_t *) ti_vint_create(idx);
            if (!query->rval)
                ex_set_mem(e);

            goto done;
        }

        query->rval = NULL;
    }

    query->rval = (ti_val_t *) ti_nil_get();

done:
fail2:
    ti_closure_dec(closure, query);

fail1:
    ti_val_drop((ti_val_t *) closure);

fail0:
    ti_val_unlock((ti_val_t *) varr, lock_was_set);
    ti_val_drop((ti_val_t *) varr);
    return e->nr;
}
