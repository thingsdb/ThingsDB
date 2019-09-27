#include <ti/fn/fn.h>

static int do__f_isarray(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool is_array;

    if (fn_chained("isarray", query, e) ||
        fn_nargs("isarray", DOC_ISARRAY, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_array = ti_val_is_array(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_array);

    return e->nr;
}
