#include <ti/fn/fn.h>

static int do__f_trim_left(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_raw_t * raw;

    if (!ti_val_is_str(query->rval))
        return fn_call_try("trim_left", query, nd, e);

    if (fn_nargs("trim_left", DOC_STR_TRIM_LEFT, 0, nargs, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    query->rval = (ti_val_t *) ti_str_trim_left(raw);
    if (!query->rval)
        ex_set_mem(e);

    ti_val_unsafe_drop((ti_val_t *) raw);
    return e->nr;
}
