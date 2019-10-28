#include <ti/fn/fn.h>

static int do__f_type(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_val_t * type_str;

    if (fn_chained("type", query, e) ||
        fn_nargs("type", DOC_TYPE, 1, nargs, e))
        return e->nr;

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    type_str = ti_val_strv(query->rval);
    ti_val_drop(query->rval);
    query->rval = type_str;

    return e->nr;
}
