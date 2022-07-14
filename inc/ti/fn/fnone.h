#include <ti/fn/fn.h>

static int do__f_one(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_vset_t * vset;
    ti_val_t * value;

    if (!ti_val_is_set(query->rval))
        return fn_call_try("one", query, nd, e);

    if (fn_nargs("one", DOC_SET_ONE, 0, nargs, e))
        return e->nr;

    vset = (ti_vset_t *) query->rval;
    value = imap_one(vset->imap);

    if (!value)
    {
        ex_set(e, EX_LOOKUP_ERROR, "one from empty set");
        return e->nr;
    }

    query->rval = value;
    ti_incref(value);

    ti_val_unsafe_drop((ti_val_t *) vset);
    return e->nr;
}
