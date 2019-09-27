#include <ti/fn/fn.h>

static int do__f_isnil(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool is_nil;

    if (fn_chained("isnil", query, e) ||
        fn_nargs("isnil", DOC_ISNIL, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_nil = ti_val_is_nil(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_nil);

    return e->nr;
}
