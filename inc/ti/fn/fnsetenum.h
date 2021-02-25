#include <ti/fn/fn.h>

static int do__f_set_enum(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_enum_t * enum_;
    ti_thing_t * thing;
    ti_task_t * task;
    ti_raw_t * rname;
    uint16_t enum_id;
    uint64_t ts_now = util_now_usec();

    if (fn_not_collection_scope("set_enum", query, e) ||
        fn_nargs("set_enum", DOC_SET_ENUM, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("set_enum", DOC_SET_ENUM, 1, query->rval, e))
        return e->nr;

    rname = (ti_raw_t *) query->rval;
    if (!ti_name_is_valid_strn((const char *) rname->data, rname->n))
    {
        ex_set(e, EX_VALUE_ERROR,
            "function `set_enum` expects "
            "argument 1 to be a valid enum name"DOC_NAMES);
        return e->nr;
    }

    if (ti_spec_is_reserved((const char *) rname->data, rname->n))
    {
        ex_set(e, EX_VALUE_ERROR,
                "name `%.*s` is reserved",
                (int) rname->n, (const char *) rname->data);
        return e->nr;
    }

    if (ti_enums_by_raw(query->collection->enums, rname))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "enum `%.*s` already exists",
                (int) rname->n, (const char *) rname->data);
        return e->nr;
    }

    if (ti_types_by_raw(query->collection->types, rname))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%.*s` already exists",
                (int) rname->n, (const char *) rname->data);
        return e->nr;
    }

    enum_id = ti_enums_get_new_id(query->collection->enums, e);
    if (e->nr)
        return e->nr;

    enum_ = ti_enum_create(
            enum_id,
            (const char *) rname->data,
            rname->n,
            ts_now,
            0);

    if (!enum_)
    {
        ex_set_mem(e);
        return e->nr;
    }

    if (ti_enums_add(query->collection->enums, enum_))
    {
        ex_set_mem(e);
        goto fail0;
    }

    if (ti_enum_try_lock(enum_, e))
        goto fail1;

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next->node, e) ||
        fn_arg_thing("set_enum", DOC_SET_ENUM, 2, query->rval, e))
        goto fail2;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_enum_init_from_thing(enum_, thing, e))
        goto fail3;

    task = ti_task_get_task(query->ev, query->collection->root);
    if (!task || ti_task_add_set_enum(task, enum_))
    {
        ex_set_mem(e);
        goto fail3;
    }

    query->rval = (ti_val_t *) ti_nil_get();

fail3:
    ti_val_unsafe_drop((ti_val_t *) thing);
fail2:
    ti_enum_unlock(enum_, true /* lock is set for sure */);
fail1:
    if (e->nr)
        ti_enums_del(query->collection->enums, enum_);
fail0:
    if (e->nr)
        ti_enum_destroy(enum_);

    return e->nr;
}
