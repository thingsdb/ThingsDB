#include <ti/fn/fn.h>

static int do__f_root(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    if (fn_nargs("root", DOC_ROOT, 0, nargs, e))
        return e->nr;

    if (query->collection)
    {
        query->rval = (ti_val_t *) query->collection->root;
        ti_incref(query->rval);
    }
    else
        query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
