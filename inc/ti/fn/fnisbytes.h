#include <ti/fn/fn.h>

static int do__f_is_bytes(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool is_bytes;

    if (fn_nargs("is_bytes", DOC_IS_BYTES, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_bytes = ti_val_is_bytes(query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_bytes);

    return e->nr;
}

static int do__f_isbytes(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    log_error("function `isbytes` is deprecated, use `is_bytes` instead");
    return do__f_is_bytes(query, nd, e);
}
