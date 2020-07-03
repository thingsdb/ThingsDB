#include <ti/fn/fn.h>

static int do__f_procedure_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_procedure_t * procedure;
    vec_t * procedures = ti_query_procedures(query);

    if (fn_not_thingsdb_or_collection_scope("procedure_info", query, e) ||
        fn_nargs("procedure_info", DOC_PROCEDURE_INFO, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("procedure_info", DOC_PROCEDURE_INFO, 1, query->rval, e))
        return e->nr;

    procedure = ti_procedures_by_name(procedures, (ti_raw_t *) query->rval);
    if (!procedure)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "procedure", e);

    ti_val_unsafe_drop(query->rval);
    query->rval = ti_procedure_as_mpval(procedure, ti_access_check(
            ti_query_access(query),
            query->user,
            TI_AUTH_MODIFY));

    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
