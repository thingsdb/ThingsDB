#include <ti/fn/fn.h>

static int do__f_else(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_future_t * future;

    if (!ti_val_is_future(query->rval))
        return fn_call_try("else", query, nd, e);

    future = (ti_future_t *) query->rval;
    query->rval = NULL;

    if (future->fail)
    {
        ex_set(e, EX_OPERATION, "only one `else` case is allowed");
        goto fail;
    }

    if (fn_nargs("else", DOC_FUTURE_ELSE, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_closure("else", DOC_FUTURE_ELSE, 1, query->rval, e) ||
        ti_closure_unbound((ti_closure_t *) query->rval, e) ||
        ti_closure_inc_future((ti_closure_t *) query->rval, e))
        goto fail;

    future->fail = (ti_closure_t *) query->rval;
    query->rval = (ti_val_t *) future;

    return e->nr;

fail:
    ti_val_unsafe_drop((ti_val_t *) future);
    return e->nr;
}
