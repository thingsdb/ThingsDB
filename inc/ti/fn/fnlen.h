#include <ti/fn/fn.h>

static int do__f_len(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_val_t * val;

    if (fn_not_chained("len", query, e))
        return e->nr;

    if (!ti_val_has_len(query->rval))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no function `len`"DOC_LEN,
                ti_val_str(query->rval));
        return e->nr;
    }

    if (fn_nargs("len", DOC_LEN, 0, nargs, e))
        return e->nr;

    val = query->rval;
    query->rval = (ti_val_t *) ti_vint_create((int64_t) ti_val_get_len(val));
    if (!query->rval)
        ex_set_mem(e);

    ti_val_drop(val);
    return e->nr;
}
