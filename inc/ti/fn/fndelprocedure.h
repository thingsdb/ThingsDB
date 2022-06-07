#include <ti/fn/fn.h>

static int do__f_del_procedure(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_procedure_t * procedure;
    ti_task_t * task;
    smap_t * procedures = ti_query_procedures(query);

    if (fn_not_thingsdb_or_collection_scope("del_procedure", query, e) ||
        fn_nargs("del_procedure", DOC_DEL_PROCEDURE, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_str("del_procedure", DOC_DEL_PROCEDURE, 1, query->rval, e))
        return e->nr;

    procedure = ti_procedures_by_name(procedures, (ti_raw_t *) query->rval);
    if (!procedure)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "procedure", e);

    (void) ti_procedures_pop(procedures, procedure);

    ti_procedure_destroy(procedure);

    task = ti_task_get_task(
            query->change,
            query->collection ? query->collection->root : ti.thing0);
    if (!task || ti_task_add_del_procedure(task, (ti_raw_t *) query->rval))
        ex_set_mem(e);  /* task cleanup is not required */

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
