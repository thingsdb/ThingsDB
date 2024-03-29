#include <ti/fn/fn.h>

static int do__f_set_type(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_type_t * type;
    ti_thing_t * thing;
    ti_task_t * task;
    ssize_t n;
    uint64_t ts_now = util_now_usec();
    cleri_node_t * child = nd->children;
    _Bool is_new_type = false;
    _Bool wpo = false;
    _Bool hid = false;

    if (fn_not_collection_scope("set_type", query, e) ||
        fn_nargs_range("set_type", DOC_SET_TYPE, 2, 4, nargs, e) ||
        ti_do_statement(query, child, e) ||
        fn_arg_str("set_type", DOC_SET_TYPE, 1, query->rval, e))
        return e->nr;

    type = ti_types_by_raw(query->collection->types, (ti_raw_t *) query->rval);
    if (!type)
    {
        uint16_t type_id;
        ti_raw_t * rname = (ti_raw_t *) query->rval;

        if (ti_spec_is_reserved((const char *) rname->data, rname->n))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "name `%.*s` is reserved",
                    rname->n, (const char *) rname->data);
            return e->nr;
        }

        if (ti_enums_by_raw(query->collection->enums, rname))
        {
            ex_set(e, EX_LOOKUP_ERROR,
                    "enum `%.*s` already exists",
                    rname->n, (const char *) rname->data);
            return e->nr;
        }

        if (!ti_name_is_valid_strn((const char *) rname->data, rname->n))
        {
            ex_set(e, EX_VALUE_ERROR,
                "function `set_type` expects "
                "argument 1 to be a valid type name"DOC_NAMES);
            return e->nr;
        }

        type_id = ti_types_get_new_id(query->collection->types, rname, e);
        if (e->nr)
            return e->nr;

        type = ti_type_create(
                query->collection->types,
                type_id,
                TI_TYPE_FLAG_WRAP_ONLY,  /* prevents looking for instances */
                (const char *) rname->data,
                rname->n,
                ts_now,
                0);

        if (!type)
        {
            ex_set_mem(e);
            return e->nr;
        }
        is_new_type = true;
    }
    else if (type->fields->n)
    {
        ex_set(e, EX_OPERATION,
            "function `set_type` works only on a new type; "
            "use `mod_type()` if you want to change an existing type"
            DOC_MOD_TYPE);
        return e->nr;
    }

    if (ti_type_try_lock(type, e))
        goto fail0;

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, (child = child->next->next), e) ||
        fn_arg_thing("set_type", DOC_SET_TYPE, 2, query->rval, e))
        goto fail1;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (nargs >= 3)
    {
        if (ti_do_statement(query, (child = child->next->next), e) ||
            fn_arg_bool("set_type", DOC_SET_TYPE, 3, query->rval, e))
            goto fail2;

        wpo = ti_val_as_bool(query->rval);
        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;

        if (nargs == 4)
        {
            if (ti_do_statement(query, (child = child->next->next), e) ||
                fn_arg_bool("set_type", DOC_SET_TYPE, 4, query->rval, e))
                goto fail2;

            hid = ti_val_as_bool(query->rval);
            ti_val_unsafe_drop(query->rval);
            query->rval = NULL;
        }
    }

    n = ti_query_count_type(query, type);
    if (n)
    {
        if (n < 0)
            ex_set_mem(e);
        else
            ex_set(e, EX_OPERATION,
                "function `set_type` can only be used on a type without active "
                "instances; %zd active instance%s of type `%s` %s been found"
                DOC_SET_TYPE,
                n, n == 1 ? "" : "s", type->name, n == 1 ? "has" : "have");
        goto fail2;
    }

    if (nargs >= 3)
    {
        if (wpo && ti_type_required_by_non_wpo(type, e))
            goto fail2;

        /* It is important to set wrap-only mode before initializing,
         * since field dependencies might depend on the correct wrap-only
         * setting.
         */
        ti_type_set_wrap_only_mode(type, wpo);
        if (nargs == 4)
            ti_type_set_hide_id(type, hid);
    }
    else if (is_new_type)
        ti_type_set_wrap_only_mode(type, false);

    if (ti_type_init_from_thing(type, thing, e))
        goto fail2;

    task = ti_task_get_task(query->change, query->collection->root);
    if (!task)
    {
        ex_set_mem(e);
        goto fail2;
    }
    /* update modified time-stamp if this is an existing type */
    if (!is_new_type)
        type->modified_at = ts_now;

    if ((is_new_type && ti_task_add_new_type(task, type)) ||
        ti_task_add_set_type(task, type))
    {
        ex_set_mem(e);
        goto fail2;
    }

    if (!is_new_type)
        /* only required when this is an existing type, note that nodes which
         * run this from a CPKG make this call anyway */
        ti_type_map_cleanup(type);

    is_new_type = false;  /* set always to false to prevent cleanup */
    query->rval = (ti_val_t *) ti_nil_get();

fail2:
    ti_val_unsafe_drop((ti_val_t *) thing);
fail1:
    ti_type_unlock(type, true /* lock is set for sure */);
fail0:
    if (is_new_type)
        ti_type_drop(type);

    return e->nr;
}
