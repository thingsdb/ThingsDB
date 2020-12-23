#include <ti/fn/fn.h>

static int do__f_ends_with(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * raw;
    _Bool ends_with;

    if (!ti_val_is_str(query->rval))
        return fn_call_try("ends_with", query, nd, e);

    if (fn_nargs("ends_with", DOC_STR_ENDS_WITH, 1, nargs, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("ends_with", DOC_STR_ENDS_WITH, 1, query->rval, e))
        goto failed;

    ends_with = ti_raw_endswith(raw, (ti_raw_t *) query->rval);
    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(ends_with);

failed:
    ti_val_unsafe_drop((ti_val_t *) raw);
    return e->nr;
}
