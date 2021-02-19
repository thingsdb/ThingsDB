#include <ti/fn/fn.h>

static int do__f_has_type(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    _Bool has_type;

    if (fn_not_collection_scope("has_type", query, e) ||
        fn_nargs("has_type", DOC_HAS_TYPE, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("has_type", DOC_HAS_TYPE, 1, query->rval, e))
        return e->nr;

    has_type = !!ti_types_by_raw(
            query->collection->types,
            (ti_raw_t *) query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(has_type);

    return e->nr;
}
