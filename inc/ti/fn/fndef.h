#include <ti/fn/fn.h>

static int do__f_def(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * def;

    if (!ti_val_is_closure(query->rval))
        return fn_call_try("def", query, nd, e);

    if (fn_nargs("def", DOC_CLOSURE_DEF, 0, nargs, e))
        return e->nr;

    def = ti_closure_def((ti_closure_t *) query->rval);
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) def;

    return e->nr;
}
