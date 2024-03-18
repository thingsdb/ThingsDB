#include <ti/fn/fn.h>

static int do__f_set_default_deep(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_task_t * task;
    uint8_t deep;
    uint64_t scope_id;
    vec_t ** access_;

    if (fn_not_thingsdb_scope("set_default_deep", query, e) ||
        fn_nargs("set_default_deep", DOC_SET_DEFAULT_DEEP, 2, nargs, e) ||
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
        ti_deep_from_val(query->rval, &deep, e))
        return e->nr;

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    task = ti_task_get_task(query->change, ti.thing0);

    if (!task || ti_task_add_set_default_deep(task, scope_id, deep))
        ex_set_mem(e);  /* task cleanup is not required */
    else
        ti_scope_set_deep(scope_id, deep);

    return e->nr;
}
