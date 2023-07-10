#include <ti/fn/fn.h>

static int do__f_count(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    size_t count = 0;
    ti_varr_t * varr;

    if (!ti_val_is_array(query->rval))
        return fn_call_try("count", query, nd, e);

    if (fn_nargs("count", DOC_LIST_COUNT, 1, nargs, e))
        return e->nr;

    varr = (ti_varr_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e))
        goto fail0;

    if (ti_val_is_closure(query->rval))
    {
        int64_t idx = 0;
        ti_closure_t * closure = (ti_closure_t *) query->rval;
        int lock_was_set = ti_val_ensure_lock((ti_val_t *) varr);
        query->rval = NULL;

        if (ti_closure_try_wse(closure, query, e) ||
            ti_closure_inc(closure, query, e))
            goto fail1;

        for (vec_each(VARR(varr), ti_val_t, v), ++idx)
        {
            if (ti_closure_vars_val_idx(closure, v, idx))
            {
                ex_set_mem(e);
                goto fail2;
            }

            if (ti_closure_do_statement(closure, query, e))
                goto fail2;

            count += ti_val_as_bool(query->rval);
            ti_val_unsafe_drop(query->rval);
            query->rval = NULL;
        }

fail2:
        ti_closure_dec(closure, query);
fail1:
        ti_val_unsafe_drop((ti_val_t *) closure);
        ti_val_unlock((ti_val_t *) varr, lock_was_set);
    }
    else
    {
        for (vec_each(varr->vec, ti_val_t, v))
            count += ti_opr_eq(v, query->rval);
        ti_val_unsafe_drop(query->rval);
    }

    query->rval = (ti_val_t *) ti_vint_create(count);
    if (!query->rval)
        ex_set_mem(e);

fail0:
    ti_val_unsafe_drop((ti_val_t *) varr);
    return e->nr;
}
