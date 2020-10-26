#include <ti/fn/fn.h>

static int do__f_is_enum(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool is_enum;

    if (fn_nargs("is_enum", DOC_IS_ENUM, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_enum = ti_val_is_member(query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_enum);

    return e->nr;
}

static int do__f_isenum(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    log_debug("function `isenum` is deprecated, use `is_enum` instead");
    return do__f_is_enum(query, nd, e);
}
