#include <ti/rfn/fn.h>

static int rq__f_new_collection(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_thingsdb(query, nd, e));
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    ti_raw_t * rname;
    ti_collection_t * collection;
    ti_task_t * task;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `new_collection` takes 1 argument but %d were given",
                n);
        return e->nr;
    }

    if (rq__scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `new_collection` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead",
            ti_val_str(query->rval));
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
        ex_set_alloc(e);  /* task cleanup is not required */

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vint_create((int64_t) collection->root->id);
    if (!query->rval)
        ex_set_alloc(e);

finish:
    return e->nr;
}
