#include <ti/fn/fn.h>

static int do__f_is_future(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    _Bool is_future;

    if (fn_nargs("is_future", DOC_IS_FUTURE, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e))
        return e->nr;

    is_future = ti_val_is_future(query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_future);

    return e->nr;
}
