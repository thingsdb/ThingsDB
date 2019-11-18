#include <ti/fn/fn.h>

static int do__f_doc(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * doc;

    if (fn_not_chained("doc", query, e))
        return e->nr;

    if (!ti_val_is_closure(query->rval))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no function `doc`",
                ti_val_str(query->rval));
        return e->nr;
    }

    if (fn_nargs("doc", DOC_CLOSURE_DOC, 0, nargs, e))
        return e->nr;

    doc = ti_closure_doc((ti_closure_t *) query->rval);
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) doc;

    return e->nr;
}
