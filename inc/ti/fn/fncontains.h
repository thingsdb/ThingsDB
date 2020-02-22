#include <ti/fn/fn.h>

static int do__f_contains(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * raw;
    _Bool contains;

    if (!ti_val_is_str(query->rval))
        return fn_call_try("contains", query, nd, e);

    if (fn_nargs("contains", DOC_STR_CONTAINS, 1, nargs, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("contains", DOC_STR_CONTAINS, 1, query->rval, e))
        goto failed;

    contains = ti_raw_contains(raw, (ti_raw_t *) query->rval);
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(contains);

failed:
    ti_val_drop((ti_val_t *) raw);
    return e->nr;
}
