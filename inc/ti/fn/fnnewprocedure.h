#include <ti/fn/fn.h>

static int do__f_new_procedure(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    int rc;
    ti_raw_t * raw;
    ti_task_t * task;
    ti_procedure_t * procedure;
    ti_closure_t * closure;
    vec_t ** procedures = query->collection
            ? &query->collection->procedures
            : &ti()->procedures;

    if (fn_not_thingsdb_or_collection_scope("new_procedure", query, e) ||
        fn_nargs("new_procedure", DOC_NEW_PROCEDURE, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `new_procedure` expects argument 1 to be of "
            "type `"TI_VAL_STR_S"` but got type `%s` instead"
            DOC_NEW_PROCEDURE,
            ti_val_str(query->rval));
        return e->nr;
    }

    raw = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (!ti_name_is_valid_strn((const char *) raw->data, raw->n))
    {
        ex_set(e, EX_VALUE_ERROR,
                "procedure name must follow the naming rules"DOC_NAMES);
        goto fail0;
    }

    if (ti_do_statement(query, nd->children->next->next->node, e))
        goto fail0;;

    if (!ti_val_is_closure(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `new_procedure` expects argument 2 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead"
                DOC_NEW_PROCEDURE,
                ti_val_str(query->rval));
        goto fail0;
    }

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (ti_closure_unbound(closure, e))
        goto fail1;


    procedure = ti_procedure_create(raw, closure, util_now_tsec());
    if (!procedure)
        goto alloc_error;

    rc = ti_procedures_add(procedures, procedure);
    if (rc < 0)
        goto alloc_error;
    if (rc > 0)
    {
        ex_set(e, EX_LOOKUP_ERROR, "procedure `%.*s` already exists",
                (int) procedure->name->n, (char *) procedure->name->data);
        goto fail2;
    }

    task = ti_task_get_task(
            query->ev,
            query->collection ? query->collection->root : ti()->thing0,
            e);
    if (!task)
        goto undo;

    if (ti_task_add_new_procedure(task, procedure))
        goto undo;

    query->rval = (ti_val_t *) procedure->name;
    ti_incref(query->rval);

    goto done;

undo:
    (void) vec_pop(*procedures);

alloc_error:
    if (!e->nr)
        ex_set_mem(e);

fail2:
    ti_procedure_destroy(procedure);

done:
fail1:
    ti_val_drop((ti_val_t *) closure);

fail0:
    ti_val_drop((ti_val_t *) raw);
    return e->nr;
}
