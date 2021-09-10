#include <ti/fn/fn.h>

static int do__f_change_id(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    if (fn_nargs("change_id", DOC_CHANGE_ID, 0, nargs, e))
        return e->nr;

    query->rval = query->change
            ? (ti_val_t *) ti_vint_create((int64_t) query->change->id)
            : (ti_val_t *) ti_nil_get();

    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
