#include <ti/fn/fn.h>

static void type__add(
        ti_query_t * query,
        ti_type_t * type,
        ti_name_t * name,
        cleri_node_t * nd,
        ex_t * e)
{
    static const char * fnname = "mod_type` with task `add";
    size_t n = 0;
    cleri_children_t * child;
    ti_task_t * task;
    ti_raw_t * spec_raw;
    ti_field_t * field = ti_field_by_name(type, name);
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_nargs_range(fnname, DOC_MOD_TYPE_ADD, 4, 5, nargs, e))
        return;

    if (field)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "property `%s` already exist on type `%s`",
                name->str, type->name);
        return;
    }

    child = nd->children->next->next->next->next->next->next;
    task = ti_task_get_task(query->ev, query->collection->root, e);

    if (!task ||
        ti_do_statement(query, child->node, e) ||
        fn_arg_str(fnname, DOC_MOD_TYPE_ADD, 4, query->rval, e))
        return;

    n = ti_query_count_type(query, type);
    spec_raw = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (n)
    {
        if (nargs != 5)
        {
            ex_set(e, EX_OPERATION_ERROR,
                "function `%s` requires an initial value when "
                "adding a property to a type with one or more instances; "
                "%zu active instance%s of type `%s` %s been found"DOC_MOD_TYPE_ADD,
                fnname, n, n == 1 ? "" : "s", type->name,
                n == 1 ? "has" : "have");
            goto fail0;
        }

        /*
         * The initial value must be read before creating the new field so
         * the scope will be handled according the `old` type for the case
         * where a reference to the own type is made.
         */
        child = child->next->next;
        if (ti_do_statement(query, child->node, e))
            goto fail0;
    }

    field = ti_field_create(name, spec_raw, type, e);
    if (!field)
        goto fail0;

    ti_decref(spec_raw);

    if (n)
    {
        assert (query->rval);
        if (ti_field_make_assignable(field, &query->rval, e))
            goto fail1;

        /* here we create the ID's for optional new things */
        if (ti_val_gen_ids(query->rval))
        {
            ex_set_mem(e);
            goto fail1;
        }
    }

    /* update modified time-stamp */
    type->modified_at = util_now_tsec();

    /* query->rval might be null; when there are no instances */
    if (ti_task_add_mod_type_add(task, type, query->rval))
    {
        ex_set_mem(e);
        goto fail1;
    }

    if (n)
    {
        assert (query->rval);

        /* check for variable to update, val_cache is not required since only
         * things with an id are stored in cache
         */
        for (vec_each(query->vars, ti_prop_t, prop))
        {
            ti_thing_t * thing = (ti_thing_t *) prop->val;
            if (thing->tp == TI_VAL_THING &&
                thing->id == 0 &&
                thing->type_id == type->type_id)
            {
                if (ti_val_make_assignable(&query->rval, e))
                    goto fail1;

                if (vec_push(&thing->items, query->rval))
                {
                    ex_set_mem(e);
                    goto fail1;
                }

                ti_incref(query->rval);
            }
        }

        /*
         * This function will generate all the initial values on existing
         * instances; it must run after task generation so the task contains
         * possible self references according the `old` type definition;
         */
        if (ti_field_init_things(field, &query->rval, query->ev->id))
        {
            ex_set_mem(e);
            goto fail1;
        }
    }
    return;  /* success */

fail1:
    assert (e->nr);
    ti_field_remove(field);
    return;  /* failed */

fail0:
    ti_val_drop((ti_val_t *) spec_raw);
    return;  /* failed */
}

static void type__del(
        ti_query_t * query,
        ti_type_t * type,
        ti_name_t * name,
        cleri_node_t * nd,
        ex_t * e)
{
    static const char * fnname = "mod_type` with task `del";
    const int nargs = langdef_nd_n_function_params(nd);
    ti_field_t * field = ti_field_by_name(type, name);
    ti_task_t * task;

    if (fn_nargs(fnname, DOC_MOD_TYPE_DEL, 3, nargs, e))
        return;

    if (!field)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no property `%.*s`",
                type->name, name->str);
        return;
    }

    task = ti_task_get_task(query->ev, query->collection->root, e);
    if (!task)
        return;

    /* check for variable to update, val_cache is not required since only
     * things with an id are store in cache
     */
    for (vec_each(query->vars, ti_prop_t, prop))
    {
        ti_thing_t * thing = (ti_thing_t *) prop->val;
        if (thing->tp == TI_VAL_THING &&
            thing->id == 0 &&
            thing->type_id == type->type_id)
            ti_val_drop(vec_swap_remove(thing->items, field->idx));
    }

    if (ti_field_del(field, query->ev->id))
    {
        ex_set_mem(e);
        return;
    }

    /* update modified time-stamp */
    type->modified_at = util_now_tsec();

    if (ti_task_add_mod_type_del(task, type, name))
        ex_set_mem(e);
}

static void type__mod(
        ti_query_t * query,
        ti_type_t * type,
        ti_name_t * name,
        cleri_node_t * nd,
        ex_t * e)
{
    static const char * fnname = "mod_type` with task `mod";
    const int nargs = langdef_nd_n_function_params(nd);
    ti_field_t * field = ti_field_by_name(type, name);
    ti_task_t * task;
    size_t n;

    if (fn_nargs(fnname, DOC_MOD_TYPE_MOD, 4, nargs, e))
        return;

    if (!field)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no property `%s`",
                type->name, name->str);
        return;
    }

    if (ti_do_statement(
            query,
            nd->children->next->next->next->next->next->next->node,
            e) ||
        fn_arg_str(fnname, DOC_MOD_TYPE_MOD, 4, query->rval, e))
        return;

    n = ti_query_count_type(query, type);

    if (ti_field_mod(field, (ti_raw_t *) query->rval, n, e))
        return;

    task = ti_task_get_task(query->ev, query->collection->root, e);
    if (!task)
        return;

    /* update modified time-stamp */
    type->modified_at = util_now_tsec();

    if (ti_task_add_mod_type_mod(task, field, type->modified_at))
    {
        ex_set_mem(e);
        return;
    }
}

static int do__f_mod_type(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    ti_type_t * type;
    ti_name_t * name;
    ti_raw_t * rmod;
    cleri_children_t * child;
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_not_collection_scope("mod_type", query, e) ||
        fn_nargs_min("mod_type", DOC_MOD_TYPE, 3, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("mod_type", DOC_MOD_TYPE, 1, query->rval, e))
        return e->nr;

    type = ti_types_by_raw(query->collection->types, (ti_raw_t *) query->rval);
    if (!type)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "type", e);

    if (ti_type_try_lock(type, e))
        return e->nr;

    ti_val_drop(query->rval);
    query->rval = NULL;
    child = nd->children->next->next;

    if (ti_do_statement(query, child->node, e) ||
        fn_arg_name_check("mod_type", DOC_MOD_TYPE, 2, query->rval, e))
        goto fail0;

    rmod = (ti_raw_t *) query->rval;
    query->rval = NULL;
    child = child->next->next;

    if (ti_do_statement(query, child->node, e) ||
        fn_arg_name_check("mod_type", DOC_MOD_TYPE, 3, query->rval, e))
        goto fail1;

    name = ti_names_from_raw((ti_raw_t *) query->rval);
    if (!name)
    {
        ex_set_mem(e);
        goto fail1;
    }

    ti_val_drop(query->rval);
    query->rval = NULL;

    if (ti_raw_eq_strn(rmod, "add", 3))
    {
        type__add(query, type, name, nd, e);
        goto done;
    }

    if (ti_raw_eq_strn(rmod, "del", 3))
    {
        type__del(query, type, name, nd, e);
        goto done;
    }

    if (ti_raw_eq_strn(rmod, "mod", 3))
    {
        type__mod(query, type, name, nd, e);
        goto done;
    }

    ex_set(e, EX_VALUE_ERROR,
            "function `mod_type` expects argument 2 to be "
            "`add`, `del` or `mod` but got `%.*s` instead"DOC_MOD_TYPE,
            (int) rmod->n, (const char *) rmod->data);

done:
    if (e->nr == 0)
    {
        ti_type_map_cleanup(type);

        ti_val_drop(query->rval);
        query->rval = (ti_val_t *) ti_nil_get();
    }
    ti_name_drop(name);

fail1:
    ti_val_drop((ti_val_t *) rmod);
fail0:
    ti_type_unlock(type, true /* lock is set for sure */);
    return e->nr;
}
