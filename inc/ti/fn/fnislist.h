#include <ti/fn/fn.h>

static int do__f_is_list(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool is_list;

    if (fn_nargs("is_list", DOC_IS_LIST, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_list = ti_val_is_list(query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_list);

    return e->nr;
}

static int do__f_islist(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    log_debug("function `islist` is deprecated, use `is_list` instead");
    return do__f_is_list(query, nd, e);
}
