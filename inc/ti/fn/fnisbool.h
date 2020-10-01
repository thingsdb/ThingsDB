#include <ti/fn/fn.h>

static int do__f_is_bool(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool is_bool;

    if (fn_nargs("is_bool", DOC_IS_BOOL, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_bool = ti_val_is_bool(query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_bool);

    return e->nr;
}
