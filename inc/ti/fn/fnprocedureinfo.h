#include <ti/fn/fn.h>

static int do__f_procedure_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_procedure_t * procedure;
    vec_t * procedures = query->collection
            ? query->collection->procedures
            : ti()->procedures;

    if (fn_not_thingsdb_or_collection_scope("procedure_info", query, e) ||
        fn_nargs("procedure_info", DOC_PROCEDURE_INFO, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `procedure_info` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead"
                DOC_PROCEDURE_INFO,
                ti_val_str(query->rval));
        return e->nr;
    }

    procedure = ti_procedures_by_name(procedures, (ti_raw_t *) query->rval);
    if (!procedure)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "procedure", e);

    ti_val_drop(query->rval);
    query->rval = ti_procedure_info_as_qpval(procedure);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
