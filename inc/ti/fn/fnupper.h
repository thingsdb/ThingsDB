#include <ti/fn/fn.h>

static int do__f_upper(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * raw;

    if (!ti_val_is_str(query->rval))
        return fn_call_try("upper", query, nd, e);

    if (fn_nargs("upper", DOC_STR_UPPER, 0, nargs, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    query->rval = (ti_val_t *) ti_str_upper(raw);
    if (!query->rval)
        ex_set_mem(e);

    ti_val_unsafe_drop((ti_val_t *) raw);
    return e->nr;
}
