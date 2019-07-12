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
    ti_syntax_t syntax;
    ti_raw_t * raw;
    ti_task_t * task;
    ti_procedure_t * procedure;
    vec_t ** procedures = query->target
            ? &query->target->procedures
            : &ti()->procedures;

    syntax.nd_val_cache_n = 0;
    syntax.flags = query->syntax.flags & (
            TI_SYNTAX_FLAG_NODE|
            TI_SYNTAX_FLAG_THINGSDB|
            TI_SYNTAX_FLAG_COLLECTION);

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `new_procedure` takes 1 argument but %d were given"
                NEW_PROCEDURE_DOC_, nargs);
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

    if (!strx_is_utf8n((const char *) raw->data, raw->n))
    {
        ex_set(e, EX_BAD_DATA,
                "function `new_procedure` expects a procedure "
                "to have valid UTF8 encoding"NEW_PROCEDURE_DOC_);
        return e->nr;
    }

    procedure = ti_procedure_from_raw(raw, &syntax, e);
    if (!procedure)
        return e->nr;

    rc = ti_procedures_add(procedures, procedure);
    if (rc < 0)
        goto alloc_error;
    if (rc > 0)
    {
        ex_set(e, EX_INDEX_ERROR, "procedure `%.*s` already exists",
                (int) procedure->name->n, (char *) procedure->name->data);
        goto failed;
    }

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        goto undo;

    if (ti_task_add_new_procedure(task, procedure))
        goto undo;

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) procedure->name;
    ti_incref(query->rval);

    return e->nr;

undo:
    (void) vec_pop(*procedures);

alloc_error:
    if (!e->nr)
        ex_set_alloc(e);

failed:
    ti_procedure_destroy(procedure);
    return e->nr;
}
