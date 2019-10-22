#include <ti/fn/fn.h>

static int do__f_lower(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * raw;

    if (fn_not_chained("lower", query, e))
        return e->nr;

    if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no function `lower`",
                ti_val_str(query->rval));
        return e->nr;
    }

    if (fn_nargs("lower", DOC_STR_LOWER, 0, nargs, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    query->rval = (ti_val_t *) ti_str_lower(raw);
    if (!query->rval)
        ex_set_mem(e);

    ti_val_drop((ti_val_t *) raw);
    return e->nr;
}
