#include <ti/fn/fn.h>

static int do__f_islist(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool is_list;

    if (fn_chained("islist", query, e) ||
        fn_nargs("islist", DOC_ISLIST, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_list = ti_val_is_list(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_list);

    return e->nr;
}
