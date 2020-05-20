#include <ti/fn/fn.h>

static int do__f_has_enum(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    ti_raw_t * rname;
    _Bool has_enum;
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_not_collection_scope("has_enum", query, e) ||
        fn_nargs("has_enum", DOC_HAS_ENUM, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("has_enum", DOC_HAS_ENUM, 1, query->rval, e))
        return e->nr;

    rname = (ti_raw_t *) query->rval;

    has_enum = !!ti_enums_by_raw(query->collection->enums, rname);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(has_enum);

    return e->nr;
}
