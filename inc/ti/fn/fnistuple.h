#include <ti/fn/fn.h>

static int do__f_is_tuple(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool is_tuple;

    if (fn_nargs("is_tuple", DOC_IS_TUPLE, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_tuple = ti_val_is_tuple(query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_tuple);

    return e->nr;
}
