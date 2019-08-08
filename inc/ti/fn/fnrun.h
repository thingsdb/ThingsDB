#include <ti/fn/fn.h>

#define RUN_DOC_ TI_SEE_DOC("#run")

static int do__f_run(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    const int nargs = langdef_nd_n_function_params(nd);
    cleri_children_t * child = nd->children;    /* first in argument list */
    ti_procedure_t * procedure;
    vec_t * arguments;
    vec_t * procedures = query->target
            ? query->target->procedures
            : ti()->procedures;

    if (!nargs)
    {
        ex_set(e, EX_BAD_DATA,
                "function `run` requires at least 1 argument but 0 "
                "were given"RUN_DOC_);
        return e->nr;
    }

    if (ti_do_scope(query, child->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `run` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead"RUN_DOC_,
                ti_val_str(query->rval));
        return e->nr;
    }

    procedure = ti_procedures_by_name(procedures, (ti_raw_t *) query->rval);
    if (!procedure)
    {
        ex_set(e, EX_INDEX_ERROR, "procedure `%.*s` not found",
                (int) ((ti_raw_t *) query->rval)->n,
                (char *) ((ti_raw_t *) query->rval)->data);
        return e->nr;
    }

    arguments = vec_new(nargs);

    for (vec_each(procedure->closure->vars, ti_prop_t, prop))
    {
        child = child->next->next;

        if (ti_do_scope(query, child->node, e))
            return e->nr;

        prop->val = query->rval;
        query->rval = NULL;
    }


    if (ti_closure_try_wse(procedure->closure, query, e) ||
        ti_closure_try_lock_and_use(procedure->closure))
    {

    }

    if (procedure->closure->flags & TI_VFLAG_CLOSURE_WSE)
    {
        ex_set(e, EX_BAD_DATA,
                "procedure `%.*s` requires an event and must be called "
                "using `calle(%.*s);`"TI_SEE_DOC("#calle"),
                (int) procedure->name->n,
                (char *) procedure->name->data,
                (int) nd->len,
                nd->str);
        return e->nr;
    }

    if ((int) procedure->arguments->n != nargs-1)
    {
        ex_set(e, EX_BAD_DATA, "procedure `%.*s` takes %u %s but %d %s given",
                (int) procedure->name->n,
                (char *) procedure->name->data,
                procedure->arguments->n,
                procedure->arguments->n == 1 ? "argument" : "arguments",
                nargs-1,
                nargs-1 == 1 ? "was" : "were");
        return e->nr;
    }

    ti_val_drop(query->rval);
    query->rval = NULL;



    return ti_procedure_call(procedure, query, e);
}
