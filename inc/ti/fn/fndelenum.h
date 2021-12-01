#include <ti/fn/fn.h>

static int do__f_del_enum(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_enum_t * enum_;
    ti_task_t * task;
    uint16_t spec;

    if (fn_not_collection_scope("del_enum", query, e) ||
        fn_nargs("del_enum", DOC_DEL_ENUM, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str_slow("del_enum", DOC_DEL_ENUM, 1, query->rval, e))
        return e->nr;

    enum_ = ti_enums_by_raw(query->collection->enums, (ti_raw_t *) query->rval);
    if (!enum_)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "enum", e);

    if (enum_->refcount)
    {
        ex_set(e, EX_OPERATION,
                "enum `%s` is used by at least one type; "
                "use `types_info(..)` to find all dependencies and "
                "remove them by using `mod_type(..)` or delete the dependency "
                "types as well"DOC_DEL_ENUM DOC_MOD_TYPE DOC_TYPES_INFO,
                enum_->name);
        return e->nr;
    }

    for (vec_each(enum_->members, ti_member_t, member))
    {
        if (member->ref > 1)
        {
            ex_set(e, EX_OPERATION,
                    "enum member `%s{%s}` is still in use"DOC_DEL_ENUM,
                    enum_->name, member->name->str);
            return e->nr;
        }
    }

    /*  if we can set the lock, the enum is not in use */
    if (ti_enum_try_lock(enum_, e))
        return e->nr;

    task = ti_task_get_task(query->change, query->collection->root);
    if (!task || ti_task_add_del_enum(task, enum_))
    {
        ex_set_mem(e);  /* task cleanup is not required */
        ti_enum_unlock(enum_, true /* lock is set for sure */);
        return e->nr;
    }

    spec = enum_->enum_id | TI_ENUM_ID_FLAG;

    for (vec_each(query->vars, ti_prop_t, prop))
    {
        if (ti_val_is_thing(prop->val))
        {
            ti_thing_t * thing = (ti_thing_t *) prop->val;

            if (ti_thing_is_object(thing) &&
                    (thing->via.spec & TI_SPEC_MASK_NILLABLE) == spec)
                thing->via.spec = TI_SPEC_ANY;
        }
    }

    /* this will remove the `enum` so it cannot be used after here */
    ti_enums_del(query->collection->enums, enum_);
    ti_enum_destroy(enum_);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
