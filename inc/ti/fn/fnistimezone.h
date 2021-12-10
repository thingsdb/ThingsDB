#include <ti/fn/fn.h>

static int do__f_is_time_zone(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    _Bool is_time_zone;
    ti_raw_t * raw;

    if (fn_nargs("is_time_zone", DOC_IS_TIME_ZONE, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    is_time_zone = ti_val_is_str(query->rval) &&
            ti_tz_from_strn((const char *) raw->data, raw->n);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_time_zone);

    return e->nr;
}
