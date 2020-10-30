#include <ti/fn/fn.h>

static int do__f_is_nil(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool is_nil;

    if (fn_nargs("is_nil", DOC_IS_NIL, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_nil = ti_val_is_nil(query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_nil);

    return e->nr;
}

static int do__f_isnil(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    log_warning("function `isnil` is deprecated, use `is_nil` instead");
    return do__f_is_nil(query, nd, e);
}
