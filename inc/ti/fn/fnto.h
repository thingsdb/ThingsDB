#include <ti/fn/fn.h>

static int do__f_to(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_datetime_t * dt;

    if (!ti_val_is_datetime(query->rval))
        return fn_call_try("to", query, nd, e);

    if (fn_nargs("to", DOC_DATETIME_TO, 1, nargs, e))
        return e->nr;

    if (query->rval->ref > 1)
    {
        dt = ti_datetime_copy((ti_datetime_t *) query->rval);
        if (!dt)
        {
            ex_set_mem(e);
            return e->nr;
        }
        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }
    else
    {
        dt = (ti_datetime_t *) query->rval;
        query->rval = NULL;
    }

    if (ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("to", DOC_DATETIME_TO, 1, query->rval, e) ||
        ti_datetime_to_zone(dt, (ti_raw_t *) query->rval, e))
        goto fail0;

    query->rval = (ti_val_t *) dt;
    return e->nr;

fail0:
    ti_val_unsafe_drop((ti_val_t *) dt);
    return e->nr;
}
