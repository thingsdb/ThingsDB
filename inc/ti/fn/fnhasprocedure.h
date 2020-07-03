#include <ti/fn/fn.h>

static int do__f_has_procedure(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool has_procedure;
    vec_t * procedures = ti_query_procedures(query);

    if (fn_not_thingsdb_or_collection_scope("has_procedure", query, e) ||
        fn_nargs("has_procedure", DOC_HAS_PROCEDURE, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("has_procedure", DOC_HAS_PROCEDURE, 1, query->rval, e))
        return e->nr;

    has_procedure = !!ti_procedures_by_name(
            procedures,
            (ti_raw_t *) query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(has_procedure);

    return e->nr;
}
