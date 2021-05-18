#include <ti/fn/fn.h>

static int do__f_is_unique(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    _Bool is_unique = true;
    ti_varr_t * varr;

    if (!ti_val_is_array(query->rval))
        return fn_call_try("is_unique", query, nd, e);

    if (fn_nargs("is_unique", DOC_LIST_IS_UNIQUE, 0, nargs, e))
        return e->nr;

    varr = (ti_varr_t *) query->rval;
    for (vec_each_addr(varr->vec, ti_val_t, va))
    {
        ti_val_t ** vp = (ti_val_t **) (varr->vec)->data;

        for (; vp < va; vp++)
            if (ti_opr_eq(*va, *vp))
                break;

        if (vp != va)
        {
            is_unique = false;
            break;
        }
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_unique);

    return e->nr;
}
