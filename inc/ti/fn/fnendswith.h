#include <ti/fn/fn.h>

static int do__f_endswith(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * raw;
    _Bool endswith;

    if (!ti_val_is_str(query->rval))
        return fn_call_try("endswith", query, nd, e);

    if (fn_nargs("endswith", DOC_STR_ENDSWITH, 1, nargs, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("endswith", DOC_STR_ENDSWITH, 1, query->rval, e))
        goto failed;

    endswith = ti_raw_endswith(raw, (ti_raw_t *) query->rval);
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(endswith);

failed:
    ti_val_drop((ti_val_t *) raw);
    return e->nr;
}
