#include <ti/fn/fn.h>

static int run__timer(
        ti_query_t * query,
        cleri_children_t * child,
        ex_t * e)
{
    vec_t ** timers = ti_query_timers(query);
    ti_timer_t * timer;
    ti_closure_t * closure;

    timer = ti_timer_from_val(*timers, query->rval, e);
    if (!timer)
        return e->nr;

    ti_val_unsafe_drop((ti_val_t *) query->rval);
    query->rval = NULL;

    if (child->next && child->next->next)
    {
        ex_set(e, EX_NUM_ARGUMENTS,
                "no arguments are allowed when using `run` with a `timer`");
        return e->nr;
    }

    closure = timer->closure;

    ti_incref(closure);  /* take a reference */

    (void) ti_closure_call(closure, query, timer->args, e);

    ti_val_unsafe_drop((ti_val_t *) closure);
    return e->nr;
}

static int run__procedure(
        ti_query_t * query,
        cleri_children_t * child,
        ex_t * e)
{
    smap_t * procedures = ti_query_procedures(query);
    ti_procedure_t * procedure;
    ti_closure_t * closure;
    vec_t * args;
    size_t n;

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

static int do__f_run(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    cleri_children_t * child = nd->children;    /* first in argument list */

    if (fn_not_thingsdb_or_collection_scope("run", query, e) ||
        fn_nargs_min("run", DOC_RUN, 1, nargs, e) ||
        ti_do_statement(query, child->node, e))
        return e->nr;

    switch ((ti_val_enum) query->rval->tp)
    {
    case TI_VAL_STR:
        return run__procedure(query, child, e);
    case TI_VAL_INT:
        return run__timer(query, child, e);
    default:
        ex_set(e, EX_TYPE_ERROR,
                "function `run` expects argument 1 to be of "
                "type `"TI_VAL_STR_S"` (procedure) "
                "or `"TI_VAL_INT_S"` (timer) but got type `%s` instead"
                DOC_RUN, ti_val_str(query->rval));
    }
    return e->nr;
}
