#include <ti/fn/fn.h>

static int do__f_set_time_zone(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_task_t * task;
    ti_collection_t * collection;

    if (fn_not_thingsdb_scope("set_time_zone", query, e) ||
        fn_nargs("set_time_zone", DOC_SET_TIME_ZONE, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    collection = ti_collections_get_by_val(query->rval, e);
    if (e->nr)
        return e->nr;
    assert (collection);

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next->node, e))
        return e->nr;

    if (ti_val_is_str(query->rval))
    {
        ti_raw_t * str = (ti_raw_t *) query->rval;
        ti_tz_t * tz = ti_tz_from_strn((const char *) str->data, str->n);

        if (!tz)
        {
            ex_set(e, EX_VALUE_ERROR, "unknown time zone");
            return e->nr;
        }

        collection->tz = tz;

    }
    else if(ti_val_is_nil(query->rval))
    {
        collection->tz = ti_tz_utc();
    }
    else
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `set_time_zone` expects argument 2 to be of "
            "type `"TI_VAL_STR_S"` or type `"TI_VAL_NIL_S"` "
            "but got type `%s` instead"DOC_SET_TIME_ZONE,
            ti_val_str(query->rval));
        return e->nr;
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    task = ti_task_get_task(query->ev, ti.thing0);

    if (!task || ti_task_add_set_time_zone(task, collection))
        ex_set_mem(e);  /* task cleanup is not required */

    return e->nr;
}
