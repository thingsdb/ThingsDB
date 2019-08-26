#include <ti/fn/fn.h>

#define RENAME_COLLECTION_DOC_ TI_SEE_DOC("#rename_collection")

static int do__f_rename_collection(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    int nargs = langdef_nd_n_function_params(nd);
    ti_task_t * task;
    ti_collection_t * collection;

    if (fn_not_thingsdb_scope("rename_collection", query, e))
        return e->nr;

    if (nargs != 2)
    {
        ex_set(e, EX_BAD_DATA,
            "function `rename_collection` takes 2 arguments "
            "but %d %s given"RENAME_COLLECTION_DOC_,
            nargs, nargs == 1 ? "was" : "were");
        return e->nr;
    }

    if (ti_do_scope(query, nd->children->node, e))
        return e->nr;

    collection = ti_collections_get_by_val(query->rval, e);
    if (e->nr)
        return e->nr;
    assert (collection);

    ti_val_drop(query->rval);
    query->rval = NULL;

    if (ti_do_scope(query, nd->children->next->next->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `rename_collection` expects argument 2 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"
            RENAME_COLLECTION_DOC_, ti_val_str(query->rval));
        return e->nr;
    }

    if (ti_collection_rename(collection, (ti_raw_t *) query->rval, e))
        return e->nr;

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        return e->nr;

    if (ti_task_add_rename_collection(task, collection))
        ex_set_mem(e);  /* task cleanup is not required */

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
