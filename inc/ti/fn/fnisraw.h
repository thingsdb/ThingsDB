#include <ti/fn/fn.h>

static int do__f_israw(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool is_raw;

    if (fn_chained("israw", query, e) ||
        fn_nargs("israw", DOC_ISRAW, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_raw = ti_val_is_raw(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_raw);

    return e->nr;
}
