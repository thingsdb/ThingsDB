#include <ti/fn/fn.h>

static int do__f_type_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_type_t * type;

    if (fn_not_collection_scope("type_info", query, e) ||
        fn_nargs("type_info", DOC_TYPE_INFO, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("type_info", DOC_TYPE_INFO, 1, query->rval, e))
        return e->nr;

    type = ti_types_by_raw(query->collection->types, (ti_raw_t *) query->rval);
    if (!type)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "type", e);

    ti_val_drop(query->rval);
    query->rval = ti_type_as_mpval(type, ti_access_check(
            query->collection->access,
            query->user,
            TI_AUTH_MODIFY));

    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
