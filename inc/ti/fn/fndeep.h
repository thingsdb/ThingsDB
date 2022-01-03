#include <ti/fn/fn.h>

static int do__f_deep(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    if (fn_nargs_max("deep", DOC_DEEP, 1, nargs, e))
        return e->nr;

    if (nargs == 1)
    {
        return ti_do_statement(query, nd->children, e)
            ? e->nr
            : ti_deep_from_val(query->rval, &query->qbind.deep, e);
    }

    /* always success since 0..127 is loaded from the stack */
    query->rval = (ti_val_t *) ti_vint_create((int64_t) query->qbind.deep);
    return e->nr;
}
