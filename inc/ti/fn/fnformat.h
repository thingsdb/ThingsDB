#include <ti/fn/fn.h>

static int do__f_format(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_datetime_t * dt;
    ti_raw_t * fmt;

    if (!ti_val_is_datetime(query->rval))
        return fn_call_try("format", query, nd, e);

    if (fn_nargs("format", DOC_DATETIME_FORMAT, 1, nargs, e))
        return e->nr;

    dt = (ti_datetime_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e) ||
        fn_arg_str("format", DOC_DATETIME_FORMAT, 1, query->rval, e))
        goto fail;

    fmt = (ti_raw_t *) query->rval;
    query->rval = (ti_val_t *) ti_datetime_to_str_fmt(dt, fmt, e);
    ti_val_unsafe_drop((ti_val_t *) fmt);

fail:
    ti_val_unsafe_drop((ti_val_t *) dt);
    return e->nr;
}
