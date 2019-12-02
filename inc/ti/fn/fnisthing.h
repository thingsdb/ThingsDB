#include <ti/fn/fn.h>

static int do__f_isthing(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool is_thing;

    if (fn_nargs("isthing", DOC_ISTHING, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_thing = ti_val_is_thing(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_thing);

    return e->nr;
}
