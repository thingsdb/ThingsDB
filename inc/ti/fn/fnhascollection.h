#include <ti/fn/fn.h>

static int do__f_has_collection(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    if (fn_not_thingsdb_scope("has_collection", query, e) ||
        fn_nargs("has_collection", DOC_HAS_COLLECTION, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e))
        return e->nr;

    (void) ti_collections_get_by_val(query->rval, e);
    if (e->nr == EX_TYPE_ERROR)
        return e->nr;

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(e->nr == 0);

    ex_clear(e);
    return e->nr;
}
