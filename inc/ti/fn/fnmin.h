#include <ti/fn/fn.h>

static int do__f_min(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_varr_t * varr;

    if (!ti_val_is_array(query->rval))
        return fn_call_try("min", query, nd, e);

    if (fn_nargs("min", DOC_LIST_MIN, 0, nargs, e))
        return e->nr;

    varr = (ti_varr_t *) query->rval;
    if (VARR(varr)->n == 0)
    {
        ex_set(e, EX_LOOKUP_ERROR, "min() on empty list"DOC_LIST_MIN);
        return e->nr;
    }

    query->rval = VARR(varr)->data[--VARR(varr)->n];
    for (vec_each(VARR(varr), ti_val_t, v))
        if (ti_opr_compare(v, query->rval, e) < 0)
            query->rval = v;
    VARR(varr)->n++;

    ti_incref(query->rval);

    ti_val_unsafe_drop((ti_val_t *) varr);
    return e->nr;
}
