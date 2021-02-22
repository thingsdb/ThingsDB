#include <ti/fn/fn.h>

static int do__f_refs(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    uint32_t ref;

    if (fn_nargs("refs", DOC_REFS, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    ref = query->rval->ref;
    ti_val_unsafe_drop(query->rval);

    query->rval = (ti_val_t *) ti_vint_create((int64_t) ref);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
