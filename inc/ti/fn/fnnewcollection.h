#include <ti/fn/fn.h>

static int do__f_new_collection(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * rname;
    ti_collection_t * collection;
    ti_task_t * task;

    if (fn_not_thingsdb_scope("new_collection", query, e) ||
        fn_nargs("new_collection", DOC_NEW_COLLECTION, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `new_collection` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"
            DOC_NEW_COLLECTION, ti_val_str(query->rval));
        return e->nr;
    }

    rname = (ti_raw_t *) query->rval;

    collection = ti_collections_create_collection(
            0,
            (const char *) rname->data,
            rname->n,
            query->user,
            e);
    if (!collection)
        goto finish;

    task = ti_task_get_task(query->ev, ti()->thing0, e);

    if (!task)
        goto finish;

    if (ti_task_add_new_collection(task, collection, query->user))
        ex_set_mem(e);  /* task cleanup is not required */

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vint_create((int64_t) collection->root->id);
    if (!query->rval)
        ex_set_mem(e);

finish:
    return e->nr;
}
