#include <ti/fn/fn.h>

static int do__f_extract(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_datetime_t * dt;
    ti_name_t * name;
    ti_vint_t * vint;
    struct tm tm;
    ti_thing_t * as_thing;

    if (!ti_val_is_datetime(query->rval))
        return fn_call_try("extract", query, nd, e);

    if (fn_nargs("extract", DOC_DATETIME_EXTRACT, 0, nargs, e))
        return e->nr;

    dt = (ti_datetime_t *) query->rval;

    if (ti_datetime_time(dt, &tm))
    {
        ex_set(e, EX_VALUE_ERROR, "failed to localize time");
        return e->nr;
    }

    as_thing = ti_thing_o_create(0, 7, query->collection);
    if (!as_thing)
    {
        ex_set_mem(e);
        return e->nr;
    }

    /* year */
    name = (ti_name_t *) ti_val_year_name();
    vint = ti_vint_create(tm.tm_year + 1900);
    if (!vint || !ti_thing_o_prop_add(as_thing, name, (ti_val_t *) vint))
        goto mem_error;

    /* month */
    name = (ti_name_t *) ti_val_month_name();
    vint = ti_vint_create(tm.tm_mon + 1);
    if (!vint || !ti_thing_o_prop_add(as_thing, name, (ti_val_t *) vint))
        goto mem_error;

    /* day */
    name = (ti_name_t *) ti_val_day_name();
    vint = ti_vint_create(tm.tm_mday);
    if (!vint || !ti_thing_o_prop_add(as_thing, name, (ti_val_t *) vint))
        goto mem_error;

    /* hour */
    name = (ti_name_t *) ti_val_hour_name();
    vint = ti_vint_create(tm.tm_hour);
    if (!vint || !ti_thing_o_prop_add(as_thing, name, (ti_val_t *) vint))
        goto mem_error;

    /* minute */
    name = (ti_name_t *) ti_val_minute_name();
    vint = ti_vint_create(tm.tm_min);
    if (!vint || !ti_thing_o_prop_add(as_thing, name, (ti_val_t *) vint))
        goto mem_error;

    /* second */
    name = (ti_name_t *) ti_val_second_name();
    vint = ti_vint_create(tm.tm_sec);
    if (!vint || !ti_thing_o_prop_add(as_thing, name, (ti_val_t *) vint))
        goto mem_error;

    /* gmt_offset */
    name = (ti_name_t *) ti_val_gmt_offset_name();
    vint = ti_vint_create(tm.tm_gmtoff);
    if (!vint || !ti_thing_o_prop_add(as_thing, name, (ti_val_t *) vint))
        goto mem_error;

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) as_thing;

    return e->nr;

mem_error:
    ex_set_mem(e);
    ti_decref(name);
    ti_val_drop((ti_val_t *) vint);
    ti_val_unsafe_drop((ti_val_t *) as_thing);
    return e->nr;
}
