#include <ti/fn/fn.h>

static int do__f_del_procedure(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_procedure_t * procedure;
    ti_task_t * task;
    vec_t * procedures = query->collection
            ? query->collection->procedures
            : ti()->procedures;

    if (fn_not_thingsdb_or_collection_scope("del_procedure", query, e) ||
        fn_nargs("del_procedure", DOC_DEL_PROCEDURE, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `del_procedure` expects argument 1 to be of "
                "type `"TI_VAL_STR_S"` but got type `%s` instead"
                DOC_DEL_PROCEDURE,
                ti_val_str(query->rval));
        return e->nr;
    }

    procedure = ti_procedures_pop_name(procedures, (ti_raw_t *) query->rval);
    if (!procedure)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "procedure", e);

    ti_procedure_destroy(procedure);

    task = ti_task_get_task(
            query->ev,
            query->collection ? query->collection->root : ti()->thing0,
            e);
    if (!task)
        return e->nr;

    if (ti_task_add_del_procedure(task, (ti_raw_t *) query->rval))
        ex_set_mem(e);  /* task cleanup is not required */

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
