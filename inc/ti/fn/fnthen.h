#include <ti/fn/fn.h>

static int do__f_then(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_future_t * future;
    size_t n;

    if (!ti_val_is_future(query->rval))
        return fn_call_try("then", query, nd, e);

    future = query->rval;
    query->rval = NULL;

    if (fn_nargs("then", DOC_FUTURE_THEN, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_closure("then", DOC_FUTURE_THEN, 1, query->rval, e))
        goto fail;

    /* remove previous closure, if one is set */
    ti_val_drop((ti_val_t *) future->then);

    future->then = (ti_closure_t *) query->rval;
    query->rval = future;

    return e->nr;

fail:
    ti_val_unsafe_drop((ti_val_t *) future);
    return e->nr;
}
