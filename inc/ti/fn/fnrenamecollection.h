#include <ti/fn/fn.h>

static int do__f_rename_collection(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_task_t * task;
    ti_collection_t * collection;

    if (fn_not_thingsdb_scope("rename_collection", query, e) ||
        fn_nargs("rename_collection", DOC_RENAME_COLLECTION, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    collection = ti_collections_get_by_val(query->rval, e);
    if (e->nr)
        return e->nr;
    assert (collection);

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next->node, e) ||
        fn_arg_str_slow(
                "rename_collection",
                DOC_RENAME_COLLECTION,
                2,
                query->rval,
                e) ||
        ti_collection_rename(collection, (ti_raw_t *) query->rval, e))
        return e->nr;

    task = ti_task_get_task(query->ev, ti.thing0);

    if (!task || ti_task_add_rename_collection(task, collection))
        ex_set_mem(e);  /* task cleanup is not required */

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
