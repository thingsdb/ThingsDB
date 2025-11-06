#include <ti/fn/fn.h>

static int do__f_rename_enum(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_enum_t * enum_;
    ti_task_t * task;
    ti_raw_t * oname, * nname;

    if (fn_not_collection_scope("rename_enum", query, e) ||
        fn_commit("rename_enum", query, e) ||
        fn_nargs("rename_enum", DOC_RENAME_ENUM, 2, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_str("rename_enum", DOC_RENAME_ENUM, 1, query->rval, e))
        return e->nr;

    oname = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next, e) ||
        fn_arg_str("rename_enum", DOC_RENAME_ENUM, 2, query->rval, e))
        goto fail0;

    nname = (ti_raw_t *) query->rval;
    query->rval = (ti_val_t *) ti_nil_get();

    enum_ = ti_enums_by_raw(query->collection->enums, oname);
    if (!enum_)
    {
        (void) ti_raw_err_not_found(oname, "enum", e);
        goto fail1;
    }

    if (ti_enum_try_lock(enum_, e))
        goto fail1;

    if (!ti_name_is_valid_strn((const char *) nname->data, nname->n))
    {
        ex_set(e, EX_VALUE_ERROR,
            "function `rename_enum` expects "
            "argument 2 to be a valid enum name"DOC_NAMES);
        goto fail2;
    }

    if (ti_spec_is_reserved((const char *) nname->data, nname->n))
    {
        ex_set(e, EX_VALUE_ERROR,
                "name `%.*s` is reserved",
                nname->n, (const char *) nname->data);
        goto fail2;
    }

    if (ti_types_by_raw(query->collection->types, nname))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%.*s` already exists",
                nname->n, (const char *) nname->data);
        goto fail2;
    }

    if (ti_enums_by_raw(query->collection->enums, nname))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "enum `%.*s` already exists",
                nname->n, (const char *) nname->data);
        goto fail2;
    }

    task = ti_task_get_task(query->change, query->collection->root);
    if (!task ||
        ti_enums_rename(query->collection->enums, enum_, nname) ||
        ti_task_add_rename_enum(task, enum_))
        ex_set_mem(e);

fail2:
    ti_enum_unlock(enum_, true /* lock is set for sure */);
fail1:
    ti_val_unsafe_drop((ti_val_t *) nname);
fail0:
    ti_val_unsafe_drop((ti_val_t *) oname);
    return e->nr;
}
