#include <ti/fn/fn.h>

static int do__f_weekday(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    int weekday;

    if (!ti_val_is_datetime(query->rval))
        return fn_call_try("weekday", query, nd, e);

    if (fn_nargs("weekday", DOC_DATETIME_WEEKDAY, 0, nargs, e))
        return e->nr;

    weekday = ti_datetime_weekday((ti_datetime_t *) query->rval);
    if (weekday < 0)
    {
        ex_set(e, EX_OVERFLOW, "date/time overflow");
        return e->nr;
    }

    ti_val_unsafe_drop(query->rval);  /* this destroys `raw_unit` */
    query->rval = (ti_val_t *) ti_vint_create(weekday);
    /* no need to check for errors since a weekday number can't fail */
    return e->nr;
}
