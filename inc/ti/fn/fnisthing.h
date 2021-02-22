#include <ti/fn/fn.h>

static int do__f_is_thing(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    _Bool is_thing;

    if (fn_nargs("is_thing", DOC_IS_THING, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_thing = ti_val_is_thing(query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_thing);

    return e->nr;
}
