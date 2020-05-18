#include <ti/fn/fn.h>

static int do__f_new_type(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * rname;
    ti_type_t * type;
    ti_task_t * task;
    uint16_t type_id;

    if (fn_not_collection_scope("new_type", query, e) ||
        fn_nargs("new_type", DOC_NEW_TYPE, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("new_type", DOC_NEW_TYPE, 1, query->rval, e))
        return e->nr;

    rname = (ti_raw_t *) query->rval;
    if (!ti_name_is_valid_strn((const char *) rname->data, rname->n))
    {
        ex_set(e, EX_VALUE_ERROR,
            "function `new_type` expects "
            "argument 1 to be a valid type name"DOC_NAMES);
        return e->nr;
    }

    if (ti_spec_is_reserved((const char *) rname->data, rname->n))
    {
        ex_set(e, EX_VALUE_ERROR,
                "name `%.*s` is reserved",
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

    if (ti_enums_by_raw(query->collection->enums, rname))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "enum `%.*s` already exists",
                (int) rname->n, (const char *) rname->data);
        return e->nr;
    }

    type_id = ti_types_get_new_id(query->collection->types, rname, e);
    if (e->nr)
        return e->nr;

    type = ti_type_create(
            query->collection->types,
            type_id,
            (const char *) rname->data,
            rname->n,
            util_now_tsec()  /* created_at */,
            0  /* modified_at */);

    if (!type)
    {
        ex_set_mem(e);
        return e->nr;
    }

    task = ti_task_get_task(query->ev, query->collection->root, e);
    if (!task)
        goto fail;

    if (ti_task_add_new_type(task, type))
    {
        ex_set_mem(e);
        goto fail;
    }

    return e->nr;

fail:
    assert (e->nr);
    ti_type_drop(type);
    return e->nr;
}
