#include <ti/rfn/fn.h>

#define DEL_COLLECTION_DOC_ TI_SEE_DOC("#del_collection")

static int rq__f_del_collection(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_thingsdb(query, nd, e));
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    uint64_t collection_id;
    ti_collection_t * collection;
    ti_task_t * task;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `del_collection` takes 1 argument but %d were given"
                DEL_COLLECTION_DOC_, nargs);
        return e->nr;
    }

    if (ti_rq_scope(query, nd->children->node, e))
        return e->nr;

    collection = ti_collections_get_by_val(query->rval, e);
    if (e->nr)
        return e->nr;

    collection_id = collection->root->id;

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        return e->nr;

    if (ti_task_add_del_collection(task, collection_id))
        ex_set_alloc(e);  /* task cleanup is not required */
    else
        (void) ti_collections_del_collection(collection_id);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
