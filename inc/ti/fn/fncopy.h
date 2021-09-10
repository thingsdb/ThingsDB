#include <ti/fn/fn.h>

static int do__f_copy(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const char * doc;
    const int nargs = fn_get_nargs(nd);
    ti_val_t * val;
    uint8_t deep;

    doc = doc_copy(query->rval);
    if (!doc)
        return fn_call_try("copy", query, nd, e);

    if (fn_nargs_max("copy", doc, 1, nargs, e))
        return e->nr;

    if (nargs == 1)
    {
        int64_t deepi;

        val = query->rval;
        query->rval = NULL;

        if (ti_do_statement(query, nd->children->node, e) ||
            fn_arg_int("copy", doc, 1, query->rval, e))
            goto fail0;

        deepi = VINT(query->rval);
        if (deepi < 0 || deepi > TI_MAX_DEEP_HINT)
        {
            ex_set(e, EX_VALUE_ERROR,
                    "expecting a `deep` value between 0 and %d "
                    "but got %"PRId64" instead",
                    TI_MAX_DEEP_HINT, deepi);
            goto fail0;
        }

        ti_val_unsafe_drop(query->rval);

        query->rval = val;
        deep = (uint8_t) deepi;
    }
    else
        deep = 1;

    if (ti_val_copy(&query->rval, NULL, NULL, deep))
        ex_set_mem(e);

    return e->nr;

fail0:
    ti_val_unsafe_drop(val);
    return e->nr;
}
