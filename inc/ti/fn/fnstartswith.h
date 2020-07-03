#include <ti/fn/fn.h>

static int do__f_startswith(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * raw;
    _Bool startswith;

    if (!ti_val_is_str(query->rval))
        return fn_call_try("startswith", query, nd, e);

    if (fn_nargs("startswith", DOC_STR_STARTSWITH, 1, nargs, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("startswith", DOC_STR_STARTSWITH, 1, query->rval, e))
        goto failed;

    startswith = ti_raw_startswith(raw, (ti_raw_t *) query->rval);
    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(startswith);

failed:
    ti_val_unsafe_drop((ti_val_t *) raw);
    return e->nr;
}
