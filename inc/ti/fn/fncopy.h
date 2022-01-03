#include <ti/fn/fn.h>

static int do__f_copy(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const char * doc;
    const int nargs = fn_get_nargs(nd);
    ti_val_t * val;
    uint8_t deep = 1;

    doc = doc_copy(query->rval);
    if (!doc)
        return fn_call_try("copy", query, nd, e);

    if (fn_nargs_max("copy", doc, 1, nargs, e))
        return e->nr;

    if (nargs == 1)
    {
        val = query->rval;
        query->rval = NULL;

        if (ti_do_statement(query, nd->children, e) ||
            ti_deep_from_val(query->rval, &deep, e))
            goto fail0;

        ti_val_unsafe_drop(query->rval);
        query->rval = val;
    }

    if (ti_val_copy(&query->rval, NULL, NULL, deep))
        ex_set_mem(e);

    return e->nr;

fail0:
    ti_val_unsafe_drop(val);
    return e->nr;
}
