#include <ti/fn/fn.h>

static int do__f_del_type(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_type_t * type;
    ti_task_t * task;

    if (fn_not_collection_scope("del_type", query, e) ||
        fn_nargs("del_type", DOC_DEL_TYPE, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("del_type", DOC_DEL_TYPE, 1, query->rval, e))
        return e->nr;

    type = ti_types_by_raw(query->collection->types, (ti_raw_t *) query->rval);
    if (!type)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "type", e);

    if (type->refcount)
    {
        ex_set(e, EX_OPERATION,
                "type `%s` is used by at least one other type; "
                "use `types_info(..)` to find all dependencies and "
                "remove them by using `mod_type(..)` or delete the dependency "
                "types as well"DOC_DEL_TYPE DOC_MOD_TYPE DOC_TYPES_INFO,
                type->name);
        return e->nr;
    }

    if (ti_type_try_lock(type, e))
        return e->nr;

    task = ti_task_get_task(query->change, query->collection->root);
    if (!task || ti_task_add_del_type(task, type))
    {
        ex_set_mem(e);  /* task cleanup is not required */
        ti_type_unlock(type, true /* lock is set for sure */);
        return e->nr;
    }

    for (vec_each(query->vars, ti_prop_t, prop))
    {
        ti_thing_t * thing = (ti_thing_t *) prop->val;
        if (thing->tp == TI_VAL_THING &&
            thing->via.type == type)
            ti_thing_t_to_object(thing);
    }

    /* this will remove the `type` so it cannot be used after here */
    ti_type_del(type);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
