#include <ti/fn/fn.h>

static int do__f_mod_procedure(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_task_t * task;
    ti_raw_t * raw;
    ti_procedure_t * procedure;
    ti_closure_t * closure;
    smap_t * procedures = ti_query_procedures(query);

    if (fn_not_thingsdb_or_collection_scope("mod_procedure", query, e) ||
        fn_nargs("mod_procedure", DOC_MOD_PROCEDURE, 2, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_str("mod_procedure", DOC_MOD_PROCEDURE, 1, query->rval, e))
        return e->nr;

    procedure = ti_procedures_by_name(procedures, (ti_raw_t *) query->rval);
    if (!procedure)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "procedure", e);

    raw = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next, e) ||
        fn_arg_closure("mod_procedure", DOC_MOD_PROCEDURE, 2, query->rval, e))
        goto fail1;

    closure = (ti_closure_t *) query->rval;
    if (ti_closure_unbound(closure, e))
        goto fail1;

    ti_procedure_mod(procedure, closure, util_now_usec());

    query->rval = (ti_val_t *) raw;

    task = ti_task_get_task(
            query->change,
            query->collection ? query->collection->root : ti.thing0);
    if (!task || ti_task_add_mod_procedure(task, procedure))
    {
        ti_panic("failed to create mod_procedure task");
        ex_set_mem(e);
    }
    return e->nr;

fail1:
    ti_val_unsafe_drop((ti_val_t *) raw);
    return e->nr;
}
