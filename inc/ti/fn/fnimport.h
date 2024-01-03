#include <ti/fn/fn.h>
#include <util/iso8601.h>

typedef struct
{
    _Bool * import_tasks;
    ex_t * e;
} import__walk_t;

static int do__import_option(
        ti_raw_t * key,
        ti_val_t * val,
        import__walk_t * w)
{
    if (ti_raw_eq_strn(key, "import_tasks", 12))
    {
        if (!ti_val_is_bool(val))
        {
            ex_set(w->e, EX_TYPE_ERROR,
                    "import_tasks must be of type `"TI_VAL_BOOL_S"` but "
                    "got type `%s` instead"DOC_IMPORT,
                    ti_val_str(val));
            return w->e->nr;
        }
        *w->import_tasks = ti_val_as_bool(val);
        return 0;

    }

    ex_set(w->e, EX_VALUE_ERROR,
            "invalid import option `%.*s`"DOC_IMPORT, key->n, key->data);

    return w->e->nr;
}

static int do__f_import(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    _Bool import_tasks = false;
    ti_task_t * task;
    ti_raw_t * bytes;

    if (fn_not_collection_scope("import", query, e) ||
        fn_nargs_range("import", DOC_IMPORT, 1, 2, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_bytes("import", DOC_IMPORT, 1, query->rval, e))
        return e->nr;

    bytes = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (nargs == 2)
    {
        ti_thing_t * thing;

        if (ti_do_statement(query, nd->children->next->next, e) ||
            fn_arg_thing("import", DOC_IMPORT, 2, query->rval, e))
            goto fail0;

        thing = (ti_thing_t *) query->rval;

        import__walk_t w = {
                .import_tasks = &import_tasks,
                .e = e,
        };

        if (ti_thing_walk(thing, (ti_thing_item_cb) do__import_option, &w))
            goto fail0;

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;

    }

    task = ti_task_get_task(query->change, query->collection->root);
    if (!task)
    {
        ex_set_mem(e);
        goto fail0;
    }

    /* check late as the arguments are parsed at this point */
    if (ti_collection_check_empty(query->collection, e))
        goto fail0;

    switch(ti_collection_load(query->collection, bytes, e))
    {
    case EX_SUCCESS:
    case EX_BAD_DATA:
        break;  /* changes are made so we must create a task */
    default:
        goto fail0;
    }

    /* we can set the return value even when EX_BAD_DATA */
    query->rval = (ti_val_t *) ti_nil_get();

    if (ti_task_add_import(task, bytes, import_tasks))
    {
        ex_set_mem(e);  /* might overwrite EX_BAD_DATA */
    }
    else if (!import_tasks)
    {
        ti_collection_tasks_clear(query->collection);
    }
    else
    {
        /* this is a new collection root, thus a new task list */
        task = ti_task_new_task(query->change, query->collection->root);
        if (!task)
        {
            ex_set_mem(e);
            goto fail0;
        }
        for (vec_each(query->collection->vtasks, ti_vtask_t, vtask))
        {
            /* get ownership of all the tasks */
            ti_user_drop(vtask->user);
            vtask->user = query->user;
            ti_incref(query->user);
            if (ti_task_add_vtask_set_owner(task, vtask))
                ex_set_mem(e);  /* task cleanup is not required */
        }
    }

fail0:
    ti_val_unsafe_drop((ti_val_t *) bytes);
    return e->nr;
}
