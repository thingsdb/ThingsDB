#include <ti/fn/fn.h>


static int do__f_del_collection(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    uint64_t collection_id;
    ti_collection_t * collection;
    ti_task_t * task;

    if (fn_not_thingsdb_scope("del_collection", query, e) ||
        fn_nargs("del_collection", DOC_DEL_COLLECTION, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e))
        return e->nr;

    collection = ti_collections_get_by_val(query->rval, e);
    if (e->nr)
        return e->nr;

    collection_id = collection->id;

    task = ti_task_get_task(query->change, ti.thing0);
    if (!task || ti_task_add_del_collection(task, collection_id))
        ex_set_mem(e);  /* task cleanup is not required */
    else
        (void) ti_collections_del_collection(collection_id);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
