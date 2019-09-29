#include <ti/fn/fn.h>

static int do__f_del_type(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_type_t * type;
    ti_task_t * task;

    if (fn_not_collection_scope("del_type", query, e) ||
        fn_nargs("del_type", DOC_DEL_TYPE, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `del_type` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"DOC_DEL_TYPE,
            ti_val_str(query->rval));
        return e->nr;
    }

    type = ti_types_by_raw(query->collection->types, (ti_raw_t *) query->rval);
    if (!type)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "type", e);

    if (type->refcount)
    {
        ex_set(e, EX_OPERATION_ERROR,
                "type `%s` is used by at least one other type; "
                "use `types_info(..)` to find all dependencies and "
                "remove them by using `define(..)` or delete the dependencies "
                "as well"DOC_DEL_TYPE DOC_DEFINE DOC_TYPES_INFO,
                type->name);
        return e->nr;
    }

    task = ti_task_get_task(query->ev, query->collection->root, e);
    if (!task)
        return e->nr;

    if (ti_task_add_del_type(task, type))
        ex_set_mem(e);  /* task cleanup is not required */
    else
        /* this will remove the `type` so it cannot be used after here */
        ti_type_del(type);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
