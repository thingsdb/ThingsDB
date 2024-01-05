#include <ti/fn/fn.h>

static int do__f_set_time_zone(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_task_t * task;

    ti_raw_t * str;
    ti_tz_t * tz;
    uint64_t scope_id;
    vec_t ** access_;

    if (fn_not_thingsdb_scope("set_time_zone", query, e) ||
        fn_nargs("set_time_zone", DOC_SET_TIME_ZONE, 2, nargs, e) ||
        ti_do_statement(query, nd->children, e))
        return e->nr;

    access_ = ti_val_get_access(query->rval, e, &scope_id);
    if (e->nr)
    {
        ex_t ex = {0};
        ti_collection_t * collection = \
                ti_collections_get_by_val(query->rval, &ex);
        if (ex.nr)
            return e->nr;

        access_ = &collection->access;
        scope_id = collection->id;

        ex_clear(e);
    }

    if (ti_access_check_err(*access_, query->user, TI_AUTH_CHANGE, e))
        return e->nr;

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next, e) ||
        fn_arg_str_slow("set_time_zone", DOC_SET_TIME_ZONE, 2, query->rval, e))
        return e->nr;

    str = (ti_raw_t *) query->rval;
    tz = ti_tz_from_strn((const char *) str->data, str->n);

    if (!tz)
    {
        ex_set(e, EX_VALUE_ERROR, "unknown time zone"DOC_TIME_ZONES_INFO);
        return e->nr;
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    task = ti_task_get_task(query->change, ti.thing0);

    if (!task || ti_task_add_set_time_zone(task, scope_id, tz->index))
        ex_set_mem(e);  /* task cleanup is not required */
    else
        ti_scope_set_tz(scope_id, tz);

    return e->nr;
}
