#include <ti/fn/fn.h>

static int do__f_has_node(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    int64_t node_id;
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_not_thingsdb_scope("has_node", query, e) ||
        fn_nargs("has_node", DOC_HAS_NODE, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_int("has_node", DOC_HAS_NODE, 1, query->rval, e))
        return e->nr;

    node_id = VINT(query->rval);

    if (node_id < 0 || node_id > UINT32_MAX)
    {
        ex_set(e, EX_VALUE_ERROR,
            "function `has_node` expects argument 1 to be an "
            "integer value between 0 and %"PRIu32 DOC_HAS_NODE,
            UINT32_MAX);
        return e->nr;
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(
            !!ti_nodes_node_by_id((uint32_t) node_id));

    return e->nr;
}
