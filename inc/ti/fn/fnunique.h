#include <ti/fn/fn.h>

static int do__f_unique(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_varr_t * varr, * retv;

    if (!ti_val_is_array(query->rval))
        return fn_call_try("unique", query, nd, e);

    if (fn_nargs("unique", DOC_LIST_UNIQUE, 0, nargs, e))
        return e->nr;

    varr = (ti_varr_t*) query->rval;

    retv = ti_varr_create(varr->vec->n);
    if (!retv)
    {
        ex_set_mem(e);
        return e->nr;
    }

    ti_varr_set_may_flags(retv, varr);

    for (vec_each_addr(varr->vec, ti_val_t, va))
    {
        ti_val_t
            ** vp = (ti_val_t **) (retv->vec)->data,
            ** ve = vp + (retv->vec)->n;

        for (;vp < ve; vp++)
            if (ti_opr_eq(*va, *vp))
                break;

        if (vp == ve)
        {
            ti_incref(*va);
            VEC_push(retv->vec, *va);
        }
    }

    (void) vec_may_shrink(&retv->vec);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) retv;

    return e->nr;
}
