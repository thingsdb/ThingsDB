#include <ti/fn/fn.h>

static int do__f_deep(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    if (fn_nargs("deep", DOC_DEEP, 0, nargs, e))
        return e->nr;

    query->rval = (ti_val_t *) ti_vint_create((int64_t) query->qbind.deep);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
