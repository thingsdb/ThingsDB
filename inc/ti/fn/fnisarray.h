#include <ti/fn/fn.h>

static int do__f_is_array(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool is_array;

    if (fn_nargs("is_array", DOC_IS_ARRAY, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_array = ti_val_is_array(query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_array);

    return e->nr;
}

static int do__f_isarray(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    log_error("function `isarray` is deprecated, use `is_array` instead");
    return do__f_is_array(query, nd, e);
}
