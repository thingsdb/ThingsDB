#include <ti/fn/fn.h>

static int do__f_is_str(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool is_str;

    if (fn_nargs("is_str", DOC_IS_STR, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_str = ti_val_is_str(query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_str);

    return e->nr;
}

static int do__f_isstr(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    log_debug("function `isstr` is deprecated, use `is_str` instead");
    return do__f_is_str(query, nd, e);
}
