#include <ti/fn/fn.h>

static int do__f_wse(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    if (fn_nargs_max("wse", DOC_WSE, 1, nargs, e))
        return e->nr;

    if (!nargs)
    {
        query->rval = (ti_val_t *) ti_nil_get();
        return e->nr;
    }

    return ti_do_statement(query, nd->children, e);
}
