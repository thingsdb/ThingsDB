#include <ti/fn/fn.h>

static int do__f_yday(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    int yday;

    if (!ti_val_is_datetime(query->rval))
        return fn_call_try("yday", query, nd, e);

    if (fn_nargs("yday", DOC_DATETIME_YDAY, 0, nargs, e))
        return e->nr;

    yday = ti_datetime_yday((ti_datetime_t *) query->rval);
    if (yday < 0)
    {
        ex_set(e, EX_OVERFLOW, "date/time overflow");
        return e->nr;
    }

    ti_val_unsafe_drop(query->rval);  /* this destroys `raw_unit` */
    query->rval = (ti_val_t *) ti_vint_create(yday);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
