#include <ti/fn/fn.h>

static int do__f_bytes(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_chained("bytes", query, e) ||
        fn_nargs_max("bytes", DOC_INT, 1, nargs, e))
        return e->nr;

    if (nargs == 0)
    {
        query->rval = (ti_val_t *) ti_val_empty_bin();
        return e->nr;
    }

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    return ti_val_convert_to_bytes(&query->rval, e);
}
