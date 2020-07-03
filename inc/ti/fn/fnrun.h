#include <ti/fn/fn.h>

static int do__f_run(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    cleri_children_t * child = nd->children;    /* first in argument list */
    ti_procedure_t * procedure;
    ti_closure_t * closure;
    vec_t * args;
    vec_t * procedures = ti_query_procedures(query);
    size_t n;

    if (fn_not_thingsdb_or_collection_scope("run", query, e) ||
        fn_nargs_min("run", DOC_RUN, 1, nargs, e) ||
        ti_do_statement(query, child->node, e) ||
        fn_arg_str("run", DOC_RUN, 1, query->rval, e))
        return e->nr;

    procedure = ti_procedures_by_name(procedures, (ti_raw_t *) query->rval);
    if (!procedure)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "procedure", e);
    closure = procedure->closure;
    n = closure->vars->n;

    args = vec_new(n);
    if (!args)
    {
        ex_set_mem(e);
        return e->nr;
    }

    ti_val_unsafe_drop((ti_val_t *) query->rval);
    query->rval = NULL;
    ti_incref(closure);  /* take a reference */

    while (child->next && (child = child->next->next) && n)
    {
        --n;  // outside while so we do not go below zero
        if (ti_do_statement(query, child->node, e) ||
            ti_val_make_variable(&query->rval, e))
            goto failed;

        VEC_push(args, query->rval);
        query->rval = NULL;
    }

    while (n--)
        VEC_push(args, ti_nil_get());

    (void) ti_closure_call(closure, query, args, e);

failed:
    ti_val_unsafe_drop((ti_val_t *) closure);
    vec_destroy(args, (vec_destroy_cb) ti_val_unsafe_drop);
    return e->nr;
}
