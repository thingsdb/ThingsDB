#include <ti/fn/fn.h>

static int do__f_rename_type(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_type_t * type;
    ti_task_t * task;
    ti_raw_t * oname, * nname;


    if (fn_not_collection_scope("rename_type", query, e) ||
        fn_nargs("rename_type", DOC_RENAME_TYPE, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("rename_type", DOC_RENAME_TYPE, 1, query->rval, e))
        return e->nr;

    oname = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next->node, e) ||
        fn_arg_str("rename_type", DOC_RENAME_TYPE, 2, query->rval, e))
        goto fail0;

    nname = (ti_raw_t *) query->rval;
    query->rval = (ti_val_t *) ti_nil_get();

    type = ti_types_by_raw(query->collection->types, oname);
    if (!type)
    {
        (void) ti_raw_err_not_found(nname, "type", e);
        goto fail1;
    }

    if (ti_type_try_lock(type, e))
        goto fail1;

    if (!ti_name_is_valid_strn((const char *) nname->data, nname->n))
    {
        ex_set(e, EX_VALUE_ERROR,
            "function `rename_type` expects "
            "argument 2 to be a valid type name"DOC_NAMES);
        goto fail2;
    }

    if (ti_spec_is_reserved((const char *) nname->data, nname->n))
    {
        ex_set(e, EX_VALUE_ERROR,
                "name `%.*s` is reserved",
                (int) nname->n, (const char *) nname->data);
        goto fail2;
    }

    if (ti_types_by_raw(query->collection->types, nname))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%.*s` already exists",
                (int) nname->n, (const char *) nname->data);
        goto fail2;
    }

    if (ti_enums_by_raw(query->collection->enums, nname))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "enum `%.*s` already exists",
                (int) nname->n, (const char *) nname->data);
        goto fail2;
    }

    task = ti_task_get_task(query->ev, query->collection->root);
    if (!task ||
        ti_type_rename(type, nname) ||
        ti_task_add_rename_type(task, type))
        ex_set_mem(e);

fail2:
    ti_type_unlock(type, true /* lock is set for sure */);
fail1:
    ti_val_unsafe_drop((ti_val_t *) nname);
fail0:
    ti_val_unsafe_drop((ti_val_t *) oname);
    return e->nr;
}
