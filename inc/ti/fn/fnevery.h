#include <ti/fn/fn.h>

static int do__f_every(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool every = true;
    int64_t idx  = 0;
    ti_varr_t * varr;
    ti_closure_t * closure;

    if (!ti_val_is_array(query->rval))
        return fn_call_try("every", query, nd, e);

    if (fn_nargs("every", DOC_LIST_EVERY, 1, nargs, e))
        return e->nr;

    varr = (ti_varr_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e) ||
        fn_arg_closure("every", DOC_LIST_EVERY, 1, query->rval, e))
        goto fail0;

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (    ti_closure_try_wse(closure, query, e) ||
            ti_closure_inc(closure, query, e))
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

        every = ti_val_as_bool(query->rval);
        ti_val_drop(query->rval);

        if (!every)
            break;

        query->rval = NULL;
    }

    query->rval = (ti_val_t *) ti_vbool_get(every);
fail2:
    ti_closure_dec(closure, query);
fail1:
    ti_val_drop((ti_val_t *) closure);
fail0:
    ti_val_drop((ti_val_t *) varr);
    return e->nr;
}