#include <ti/fn/fn.h>

static int do__f_nse(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    if (fn_nargs_max("nse", DOC_NSE, 1, nargs, e))
        return e->nr;

    if (query->qbind.flags & TI_QBIND_FLAG_WSE)
    {
        ex_set(e, EX_OPERATION,
                "function `nse` failed; at least one side-effect is enforced"
                DOC_NSE);
        return e->nr;
    }

    if (!nargs)
    {
        query->rval = (ti_val_t *) ti_nil_get();
        return e->nr;
    }

    return ti_do_statement(query, nd->children, e);
}
