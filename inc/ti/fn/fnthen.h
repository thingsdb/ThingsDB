#include <ti/fn/fn.h>

static int do__f_then(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_future_t * future;

    if (!ti_val_is_future(query->rval))
        return fn_call_try("then", query, nd, e);

    future = (ti_future_t *) query->rval;
    query->rval = NULL;

    if (future->then)
    {
        ex_set(e, EX_OPERATION_ERROR, "only one `then` case is allowed");
        goto fail;
    }

    if (fn_nargs("then", DOC_FUTURE_THEN, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_closure("then", DOC_FUTURE_THEN, 1, query->rval, e) ||
        ti_closure_unbound((ti_closure_t *) query->rval, e) ||
        ti_closure_inc_future((ti_closure_t *) query->rval, e))
        goto fail;

    future->then = (ti_closure_t *) query->rval;
    query->rval = (ti_val_t *) future;

    return e->nr;

fail:
    ti_val_unsafe_drop((ti_val_t *) future);
    return e->nr;
}
