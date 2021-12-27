#include <ti/fn/fn.h>

static int do__f_bytes(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    if (fn_nargs_max("bytes", DOC_BYTES, 1, nargs, e))
        return e->nr;

    if (nargs == 0)
    {
        query->rval = (ti_val_t *) ti_val_empty_bin();
        return e->nr;
    }

    if (ti_do_statement(query, nd->children, e))
        return e->nr;

    return ti_val_convert_to_bytes(&query->rval, e);
}
