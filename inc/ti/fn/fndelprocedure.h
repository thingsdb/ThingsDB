#include <ti/fn/fn.h>

#define DEL_PROCEDURE_DOC_ TI_SEE_DOC("#del_procedure")

static int do__f_del_procedure(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (~query->syntax.flags & TI_SYNTAX_FLAG_NODE);  /* no node scope */
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    ti_procedure_t * procedure;
    ti_task_t * task;
    vec_t * procedures = query->target
            ? query->target->procedures
            : ti()->procedures;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `del_procedure` takes 1 argument but %d were given"
                DEL_PROCEDURE_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `del_procedure` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead"
                DEL_PROCEDURE_DOC_,
                ti_val_str(query->rval));
        return e->nr;
    }

    procedure = ti_procedures_pop_name(procedures, (ti_raw_t *) query->rval);
    if (!procedure)
    {
        ex_set(e, EX_INDEX_ERROR, "procedure `%.*s` not found",
                (int) ((ti_raw_t *) query->rval)->n,
                (char *) ((ti_raw_t *) query->rval)->data);
        return e->nr;
    }
    ti_procedure_drop(procedure);

    task = ti_task_get_task(query->ev, query->root, e);
    if (!task)
        return e->nr;

    if (ti_task_add_del_procedure(task, (ti_raw_t *) query->rval))
        ex_set_mem(e);  /* task cleanup is not required */

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
