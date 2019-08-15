#include <ti/fn/fn.h>

#define NEW_PROCEDURE_DOC_ TI_SEE_DOC("#new_procedure")

static int do__f_new_procedure(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (~query->syntax.flags & TI_SYNTAX_FLAG_NODE);  /* no node scope */
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    int rc;
    ti_raw_t * raw;
    ti_task_t * task;
    ti_procedure_t * procedure;
    ti_closure_t * closure;
    vec_t ** procedures = query->target
            ? &query->target->procedures
            : &ti()->procedures;
    int nargs = langdef_nd_n_function_params(nd);

    if (nargs != 2)
    {
        ex_set(e, EX_BAD_DATA,
                "function `new_procedure` takes 2 arguments but %d %s given"
                NEW_PROCEDURE_DOC_, nargs, nargs == 1 ? "was" : "were");
        return e->nr;
    }

    if (ti_do_scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `new_procedure` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"
            NEW_PROCEDURE_DOC_,
            ti_val_str(query->rval));
        return e->nr;
    }

    raw = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (!ti_name_is_valid_strn((const char *) raw->data, raw->n))
    {
        ex_set(e, EX_BAD_DATA,
                "procedure name must follow the naming rules"TI_SEE_DOC("#names"));
        goto fail0;
    }

    if (ti_do_scope(query, nd->children->next->next->node, e))
        goto fail0;;

    if (!ti_val_is_closure(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `new_procedure` expects argument 2 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead"
                NEW_PROCEDURE_DOC_,
                ti_val_str(query->rval));
        goto fail0;
    }

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (ti_closure_unbound(closure, e))
        goto fail1;


    procedure = ti_procedure_create(raw, closure);
    if (!procedure)
        goto alloc_error;

    rc = ti_procedures_add(procedures, procedure);
    if (rc < 0)
        goto alloc_error;
    if (rc > 0)
    {
        ex_set(e, EX_INDEX_ERROR, "procedure `%.*s` already exists",
                (int) procedure->name->n, (char *) procedure->name->data);
        goto fail2;
    }

    task = ti_task_get_task(query->ev, query->root, e);
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
