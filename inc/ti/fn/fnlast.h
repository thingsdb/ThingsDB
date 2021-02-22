#include <ti/fn/fn.h>

static int do__f_last(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_varr_t * varr;

    if (!ti_val_is_array(query->rval))
        return fn_call_try("last", query, nd, e);

    if (fn_nargs_max("last", DOC_LIST_LAST, 1, nargs, e))
        return e->nr;

    varr = (ti_varr_t *) query->rval;
    if (varr->vec->n)
    {
        query->rval = VEC_get(varr->vec, varr->vec->n-1);
        ti_incref(query->rval);
        goto done;
    }

    if (nargs == 0)
    {
        ex_set(e, EX_LOOKUP_ERROR, "no last item in an empty list");
        return e->nr;
    }

    query->rval = NULL;
    (void) ti_do_statement(query, nd->children->node, e);

done:
    ti_val_unsafe_drop((ti_val_t *) varr);
    return e->nr;
}
