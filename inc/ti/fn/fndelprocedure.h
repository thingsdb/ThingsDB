#include <ti/fn/fn.h>

#define DEL_PROCEDURE_DOC_ TI_SEE_DOC("#del_procedure")

static int do__f_del_procedure(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    ti_procedure_t * procedure;
    ti_task_t * task;
    vec_t * procedures = query->collection
            ? query->collection->procedures
            : ti()->procedures;

    if (fn_not_thingsdb_or_collection_scope("del_procedure", query, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_NUM_ARGUMENTS,
                "function `del_procedure` takes 1 argument but %d were given"
                DEL_PROCEDURE_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `del_procedure` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead"
                DEL_PROCEDURE_DOC_,
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
