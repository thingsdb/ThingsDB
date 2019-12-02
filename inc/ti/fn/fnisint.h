#include <ti/fn/fn.h>

static int do__f_isint(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool is_isint;

    if (fn_nargs("isint", DOC_ISINT, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_isint = ti_val_is_int(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_isint);

    return e->nr;
}
