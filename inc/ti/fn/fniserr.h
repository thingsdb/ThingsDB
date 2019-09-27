#include <ti/fn/fn.h>

static int do__f_iserr(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool is_error;

    if (fn_chained("iserr", query, e) ||
        fn_nargs("iserr", DOC_ISERR, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_error = ti_val_is_error(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_error);

    return e->nr;
}
