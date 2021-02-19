#include <ti/fn/fn.h>

static int do__f_week(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    int week;

    if (!ti_val_is_datetime(query->rval))
        return fn_call_try("week", query, nd, e);

    if (fn_nargs("week", DOC_DATETIME_WEEK, 0, nargs, e))
        return e->nr;

    week = ti_datetime_week((ti_datetime_t *) query->rval);
    if (week < 0)
    {
        ex_set(e, EX_OVERFLOW, "date/time overflow");
        return e->nr;
    }

    ti_val_unsafe_drop(query->rval);  /* this destroys `raw_unit` */
    query->rval = (ti_val_t *) ti_vint_create(week);
    /* no need to check for errors since a week number can't fail */
    return e->nr;
}
