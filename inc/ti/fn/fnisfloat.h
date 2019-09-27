#include <ti/fn/fn.h>

static int do__f_isfloat(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool is_float;

    if (fn_chained("isfloat", query, e) ||
        fn_nargs("isfloat", DOC_ISFLOAT, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_float = ti_val_is_float(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_float);

    return e->nr;
}
