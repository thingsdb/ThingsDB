#include <ti/fn/fn.h>

static int do__f_reverse(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_varr_t * src, * dst;

    if (!ti_val_is_array(query->rval))
        return fn_call_try("reverse", query, nd, e);

    if (fn_nargs("reverse", DOC_LIST_REVERSE, 0, nargs, e))
        return e->nr;

    src = (ti_varr_t *) query->rval;
    dst = ti_varr_create(src->vec->n);
    if (!dst)
    {
        ex_set_mem(e);
        return e->nr;
    }

    for (vec_each_rev(src->vec, ti_val_t, val))
    {
        VEC_push(dst->vec, val);
        ti_incref(val);
    }

    ti_varr_set_may_flags(dst, src);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) dst;

    return e->nr;
}
