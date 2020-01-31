#include <ti/fn/fn.h>

static int do__f_msg(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_verror_t * err;

    if (!ti_val_is_error(query->rval))
        return fn_call_try("msg", query, nd, e);

    if (fn_nargs("msg", DOC_ERROR_MSG, 0, nargs, e))
        return e->nr;

    err = (ti_verror_t *) query->rval;

    query->rval = (ti_val_t *) ti_str_create(err->msg, err->msg_n);
    if (!query->rval)
        ex_set_mem(e);

    ti_val_drop((ti_val_t *) err);
    return e->nr;
}
