#include <ti/fn/fn.h>

static int do__f_new_collection(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_raw_t * rname;
    ti_collection_t * collection;
    ti_task_t * task;

    if (fn_not_thingsdb_scope("new_collection", query, e) ||
        fn_nargs("new_collection", DOC_NEW_COLLECTION, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_str_slow("new_collection", DOC_NEW_COLLECTION, 1, query->rval, e))
        return e->nr;

    rname = (ti_raw_t *) query->rval;

    collection = ti_collections_create_collection(
            0,
            1,
            (const char *) rname->data,
            rname->n,
            util_now_usec(),
            query->user,
            e);

    if (!collection)
        goto finish;

    task = ti_task_get_task(query->change, ti.thing0);

    if (!task || ti_task_add_new_collection(task, collection, query->user))
        ex_set_mem(e);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) collection->name;
    ti_incref(query->rval);

finish:
    return e->nr;
}
