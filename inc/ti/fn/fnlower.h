#include <ti/fn/fn.h>

#define LOWER_DOC_ TI_SEE_DOC("#lower")

static int do__f_lower(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    ti_raw_t * raw;

    if (fn_not_chained("lower", query, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no function `lower`"LOWER_DOC_,
                ti_val_str(query->rval));
        return e->nr;
    }

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_NUM_ARGUMENTS,
                "function `lower` takes 0 arguments but %d %s given"LOWER_DOC_,
                nargs, nargs == 1 ? "was" : "were");
        return e->nr;
    }

    raw = (ti_raw_t *) query->rval;
    query->rval = (ti_val_t *) ti_raw_lower(raw);
    if (!query->rval)
        ex_set_mem(e);

    ti_val_drop((ti_val_t *) raw);
    return e->nr;
}
