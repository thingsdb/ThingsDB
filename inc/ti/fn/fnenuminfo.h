#include <ti/fn/fn.h>

static int do__f_enum_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_enum_t * enum_;

    if (fn_not_collection_scope("enum_info", query, e) ||
        fn_nargs("enum_info", DOC_ENUM_INFO, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_str("enum_info", DOC_ENUM_INFO, 1, query->rval, e))
        return e->nr;

    enum_ = ti_enums_by_raw(query->collection->enums, (ti_raw_t *) query->rval);
    if (!enum_)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "enum", e);

    ti_val_unsafe_drop(query->rval);
    query->rval = ti_enum_as_mpval(enum_);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
