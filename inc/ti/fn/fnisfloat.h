#include <ti/fn/fn.h>

static int do__f_is_float(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool is_float;

    if (fn_nargs("is_float", DOC_IS_FLOAT, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_float = ti_val_is_float(query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_float);

    return e->nr;
}

static int do__f_isfloat(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    log_debug("function `isfloat` is deprecated, use `is_float` instead");
    return do__f_is_float(query, nd, e);
}
