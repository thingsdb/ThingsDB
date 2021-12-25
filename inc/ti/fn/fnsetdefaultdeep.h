#include <ti/fn/fn.h>

static int do__f_set_default_deep(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_task_t * task;
    ti_collection_t * collection;
    ti_raw_t * str;
    ti_tz_t * tz;

    if (fn_not_thingsdb_scope("set_default_deep", query, e) ||
        fn_nargs("set_default_deep", DOC_SET_DEFAULT_DEEP, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    collection = ti_collections_get_by_val(query->rval, e);
    if (e->nr || ti_access_check_err(
            collection->access,
            query->user,
            TI_AUTH_CHANGE,
            e))
        return e->nr;

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next->node, e) ||
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

    if (tz != collection->tz)
    {
        collection->tz = tz;

        task = ti_task_get_task(query->change, ti.thing0);

        if (!task || ti_task_add_set_time_zone(task, collection))
            ex_set_mem(e);  /* task cleanup is not required */
    }

    return e->nr;
}
