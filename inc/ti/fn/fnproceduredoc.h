#include <ti/fn/fn.h>

static int do__f_procedure_doc(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_procedure_t * procedure;
    smap_t * procedures = ti_query_procedures(query);

    if (fn_not_thingsdb_or_collection_scope("procedure_doc", query, e) ||
        fn_nargs("procedure_doc", DOC_PROCEDURE_DOC, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("procedure_doc", DOC_PROCEDURE_DOC, 1, query->rval, e))
        return e->nr;

    procedure = ti_procedures_by_name(procedures, (ti_raw_t *) query->rval);
    if (!procedure)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "procedure", e);

    ti_val_unsafe_drop(query->rval);

    query->rval = (ti_val_t *) ti_closure_doc(procedure->closure);

    return e->nr;
}
