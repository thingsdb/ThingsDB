#include <ti/fn/fn.h>

static int do__f_now(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    if (fn_nargs("now", DOC_NOW, 0, nargs, e))
        return e->nr;

    query->rval = (ti_val_t *) ti_vfloat_create(util_now());
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
