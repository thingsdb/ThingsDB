#include <ti/fn/fn.h>

static int do__f_zone(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_tz_t * tz;

    if (!ti_val_is_datetime(query->rval))
        return fn_call_try("zone", query, nd, e);

    if (fn_nargs("zone", DOC_DATETIME_ZONE, 0, nargs, e))
        return e->nr;

    tz = ((ti_datetime_t *) query->rval)->tz;
    ti_val_unsafe_drop(query->rval);

    query->rval = tz
            ? (ti_val_t *) ti_str_create(tz->name, tz->n)
            : (ti_val_t *) ti_nil_get();

    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
