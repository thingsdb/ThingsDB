#include <ti/fn/fn.h>

static int do__f_to(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_datetime_t * dt;
    ti_raw_t * str, * fmt;

    if (!ti_val_is_datetime(query->rval))
        return fn_call_try("format", query, nd, e);

    if (fn_nargs("format", DOC_DATETIME_FORMAT, 2, nargs, e))
        return e->nr;

    dt = (ti_datetime_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("format", DOC_DATETIME_TO, 1, query->rval, e))
        goto fail0;

    str = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("format", DOC_DATETIME_TO, 2, query->rval, e))
        goto fail1;

    fmt = (ti_raw_t *) query->rval;
    query->rval = (ti_val_t *) ti_datetime_from_fmt(str, fmt, e);

    ti_val_unsafe_drop((ti_val_t *) fmt);
fail1:
    ti_val_unsafe_drop((ti_val_t *) str);
fail0:
    ti_val_unsafe_drop((ti_val_t *) dt);
    return e->nr;
}
