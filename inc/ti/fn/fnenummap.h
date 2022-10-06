#include <ti/fn/fn.h>

static int do__f_enum_map(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_enum_t * enum_;
    ti_thing_t * thing;

    if (fn_not_collection_scope("enum_map", query, e) ||
        fn_nargs("enum_map", DOC_ENUM_MAP, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_str("enum_map", DOC_ENUM_MAP, 1, query->rval, e))
        return e->nr;

    enum_ = ti_enums_by_raw(query->collection->enums, (ti_raw_t *) query->rval);
    if (!enum_)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "enum", e);

    thing = ti_thing_o_create(0, enum_->members->n, query->collection);
    if (!thing)
        goto alloc_error;

    for (vec_each(enum_->members, ti_member_t, member))
    {
        if (!ti_thing_p_prop_add(thing, member->name, member->val))
            goto alloc_error;
        ti_incref(member->name);
        ti_incref(member->val);
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) thing;
    return e->nr;

alloc_error:
    ti_val_drop((ti_val_t *) thing);
    ex_set_mem(e);
    return e->nr;
}
