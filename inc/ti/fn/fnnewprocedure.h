#include <ti/fn/fn.h>

static int do__f_new_procedure(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    int rc;
    ti_raw_t * raw;
    ti_task_t * task;
    ti_procedure_t * procedure;
    ti_closure_t * closure;
    smap_t * procedures = ti_query_procedures(query);

    if (fn_not_thingsdb_or_collection_scope("new_procedure", query, e) ||
        fn_nargs("new_procedure", DOC_NEW_PROCEDURE, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("new_procedure", DOC_NEW_PROCEDURE, 1, query->rval, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (!ti_name_is_valid_strn((const char *) raw->data, raw->n))
    {
        ex_set(e, EX_VALUE_ERROR,
                "procedure name must follow the naming rules"DOC_NAMES);
        goto fail0;
    }

    if (ti_do_statement(query, nd->children->next->next->node, e) ||
        fn_arg_closure("new_procedure", DOC_NEW_PROCEDURE, 2, query->rval, e))
        goto fail0;

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (ti_closure_unbound(closure, e))
        goto fail1;

    procedure = ti_procedure_create(
            (const char *) raw->data,
            raw->n,
            closure,
            util_now_tsec());
    if (!procedure)
        goto alloc_error;

    rc = ti_procedures_add(procedures, procedure);
    if (rc < 0)
        goto alloc_error;
    if (rc > 0)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "procedure `%s` already exists",
                procedure->name);
        goto fail2;
    }

    task = ti_task_get_task(
            query->ev,
            query->collection ? query->collection->root : ti.thing0);
    if (!task || ti_task_add_new_procedure(task, procedure))
        goto undo;

    query->rval = (ti_val_t *) raw;
    ti_incref(query->rval);

    goto done;

undo:
    (void) ti_procedures_pop(procedures, procedure);

alloc_error:
    if (!e->nr)
        ex_set_mem(e);

fail2:
    ti_procedure_destroy(procedure);

done:
fail1:
    ti_val_unsafe_drop((ti_val_t *) closure);

fail0:
    ti_val_unsafe_drop((ti_val_t *) raw);
    return e->nr;
}
