#include <ti/fn/fn.h>

static int do__f_doc(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * doc;

    if (!ti_val_is_closure(query->rval))
        return fn_call_try("doc", query, nd, e);

    if (fn_nargs("doc", DOC_CLOSURE_DOC, 0, nargs, e))
        return e->nr;

    doc = ti_closure_doc((ti_closure_t *) query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) doc;

    return e->nr;
}
