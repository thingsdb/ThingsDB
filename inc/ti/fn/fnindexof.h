#include <ti/fn/fn.h>

static int do__f_indexof(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    size_t idx = 0;
    ti_varr_t * varr;

    if (!ti_val_is_array(query->rval))
        return fn_call_try("indexof", query, nd, e);

    if (fn_nargs("indexof", DOC_LIST_INDEXOF, 1, nargs, e))
        return e->nr;

    varr = (ti_varr_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e))
        goto fail1;

    for (vec_each(varr->vec, ti_val_t, v), ++idx)
    {
        if (ti_opr_eq(v, query->rval))
        {
            ti_val_unsafe_drop(query->rval);
            query->rval = (ti_val_t *) ti_vint_create(idx);
            if (!query->rval)
                ex_set_mem(e);
            goto done;
        }
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

done:
fail1:
    ti_val_unsafe_drop((ti_val_t *) varr);
    return e->nr;
}
