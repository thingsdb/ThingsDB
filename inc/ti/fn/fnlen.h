#include <ti/fn/fn.h>

static int do__f_len(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const char * doc;
    const int nargs = langdef_nd_n_function_params(nd);
    ti_val_t * val;

    doc = doc_len(query->rval);
    if (!doc)
        return fn_call_try("len", query, nd, e);

    if (fn_nargs("len", doc, 0, nargs, e))
        return e->nr;

    val = query->rval;
    query->rval = (ti_val_t *) ti_vint_create((int64_t) ti_val_get_len(val));
    if (!query->rval)
        ex_set_mem(e);

    ti_val_unsafe_drop(val);
    return e->nr;
}
