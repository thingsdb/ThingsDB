#include <ti/fn/fn.h>

#define RUN_DOC_ TI_SEE_DOC("#run")

static int do__f_run(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    cleri_children_t * child = nd->children;    /* first in argument list */
    ti_procedure_t * procedure;
    vec_t * args;
    vec_t * procedures = query->target
            ? query->target->procedures
            : ti()->procedures;

    if (fn_not_thingsdb_or_collection_scope("run", query, e))
        return e->nr;

    if (!nargs)
    {
        ex_set(e, EX_BAD_DATA,
                "function `run` requires at least 1 argument but 0 "
                "were given"RUN_DOC_);
        return e->nr;
    }

    if (ti_do_statement(query, child->node, e))
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
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "procedure", e);

    args = vec_new(nargs-1);
    if (!args)
    {
        ex_set_mem(e);
        return e->nr;
    }

    ti_val_drop((ti_val_t *) query->rval);
    query->rval = NULL;

    while (child->next && (child = child->next->next))
    {
        if (ti_do_statement(query, child->node, e))
            goto failed;

        VEC_push(args, query->rval);
        query->rval = NULL;
    }

    (void) ti_closure_call(procedure->closure, query, args, e);

failed:
    vec_destroy(args, (vec_destroy_cb) ti_val_drop);
    return e->nr;
}
