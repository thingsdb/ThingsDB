#include <ti/fn/fn.h>


typedef struct
{
    ti_field_t * field;
    ti_closure_t * closure;
    ti_query_t * query;
    ex_t * e;
} type__mod_t;

static int type__mod_cb(ti_thing_t * thing, type__mod_t * w)
{
    ti_task_t * task;
    ti_prop_t * prop;
    ex_t ex = {0};

    if (thing->type_id != w->field->type->type_id)
        return 0;

    if (w->closure->vars->n)
    {
        ti_incref(thing);
        prop = vec_get(w->closure->vars, 0);
        ti_val_drop(prop->val);
        prop->val = (ti_val_t *) thing;
    }

    if (ti_closure_do_statement(w->closure, w->query, &ex) ||
        ti_val_is_nil(w->query->rval) ||
        w->query->rval == vec_get(thing->items, w->field->idx) ||
        ti_field_make_assignable(w->field, &w->query->rval, thing, &ex))

    {
        if (w->e->nr == 0 && ex.nr)
        {
            ex_set(w->e, EX_OPERATION_ERROR,
                    "field `%s` is added to type `%s` but at least one error "
                    "has occurred using the given callback; %s",
                    w->field->name->str,
                    w->field->type->name,
                    ex.msg);
        }

        ti_val_drop(w->query->rval);
    }
    else
    {
        ti_val_drop(vec_set(thing->items, w->query->rval, w->field->idx));

        if (thing->id)
        {
            task = ti_task_get_task(w->query->ev, thing, w->e);
            if (task && ti_task_add_set(task, w->field->name, w->query->rval))
                ex_set_mem(w->e);
        }
    }

    w->query->rval = NULL;
    return 0;
}

static void type__add(
        ti_query_t * query,
        ti_type_t * type,
        ti_name_t * name,
        cleri_node_t * nd,
        ex_t * e)
{
    static const char * fnname = "mod_type` with task `add";
    cleri_children_t * child;
    ti_task_t * task;
    ti_raw_t * spec_raw;
    ti_val_t * dval;
    ti_field_t * field = ti_field_by_name(type, name);
    ti_closure_t * closure;
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_nargs_range(fnname, DOC_MOD_TYPE_ADD, 4, 5, nargs, e))
        return;

    if (field)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "property `%s` already exists on type `%s`",
                name->str, type->name);
        return;
    }

    child = nd->children->next->next->next->next->next->next;
    task = ti_task_get_task(query->ev, query->collection->root, e);

    if (!task ||
        ti_do_statement(query, child->node, e) ||
        fn_arg_str_slow(fnname, DOC_MOD_TYPE_ADD, 4, query->rval, e))
        return;

    spec_raw = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (nargs == 5)
    {
        /*
         * Statements must be parsed before creating a new field since
         * potentially generated instances of this type must be without the
         * new field.
         */
        if (ti_do_statement(query, child->next->next->node, e))
            goto fail0;
    }

    field = ti_field_create(name, spec_raw, type, e);
    if (!field)
        goto fail0;

    ti_decref(spec_raw);

    if (query->rval && !ti_val_is_closure(query->rval))
    {
        if (ti_field_make_assignable(field, &query->rval, NULL, e))
            goto fail1;

        dval = query->rval;
        query->rval = NULL;
    }
    else
    {
        dval = ti_field_dval(field);
        if (!dval)
        {
            ex_set_mem(e);
            goto fail1;
        }
    }

    closure = (ti_closure_t *) query->rval;

    if (closure)
    {
        if (ti_closure_try_wse(closure, query, e) ||
            ti_closure_inc(closure, query, e))
            goto fail2;
    }


    /* here we create the ID's for optional new things */
    if (ti_val_gen_ids(dval))
    {
        ex_set_mem(e);
        goto fail3;
    }

    /* update modified time-stamp */
    type->modified_at = util_now_tsec();

    if (ti_task_add_mod_type_add(task, type, dval))
    {
        ex_set_mem(e);
        goto fail3;
    }

    for (vec_each(query->vars, ti_prop_t, prop))
    {
        ti_thing_t * thing = (ti_thing_t *) prop->val;
        if (thing->tp == TI_VAL_THING &&
            thing->id == 0 &&
            thing->type_id == type->type_id)
        {
            if (ti_val_make_assignable(&dval, thing, prop->name, e))
                goto panic;

            if (vec_push(&thing->items, dval))
            {
                ex_set_mem(e);
                goto panic;
            }

            ti_incref(dval);
        }
    }

    /*
     * This function will generate all the initial values on existing
     * instances; it must run after task generation so the task contains
     * possible self references according the `old` type definition;
     */
    if (ti_field_init_things(field, &dval, query->ev->id))
    {
        ex_set_mem(e);
        goto panic;
    }

    ti_val_drop(dval);

    if (closure)
    {
        type__mod_t addjob = {
                .field = field,
                .closure = closure,
                .query = query,
                .e = e,
        };

        query->rval = NULL;

        for (vec_each(query->vars, ti_prop_t, prop))
        {
            ti_thing_t * thing = (ti_thing_t *) prop->val;

            if (thing->tp == TI_VAL_THING && thing->id == 0)
                (void) type__mod_cb(thing, &addjob);
        }

        (void) imap_walk(
                query->collection->things,
                (imap_cb) type__mod_cb,
                &addjob);

        ti_closure_dec(closure, query);
        ti_val_drop((ti_val_t *) closure);
    }

    return;

panic:
    ti_panic("unrecoverable state after using mod_type(...)");

fail3:
    ti_closure_dec(closure, query);

fail2:
    ti_val_drop(dval);

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
                "type `%s` has no property `%s`",
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
        fn_arg_str_slow(fnname, DOC_MOD_TYPE_MOD, 4, query->rval, e))
        return;

    n = ti_query_count_type(query, type);

    if (ti_field_mod(field, (ti_raw_t *) query->rval, query->vars, n, e))
        return;

    task = ti_task_get_task(query->ev, query->collection->root, e);
    if (!task)
        return;

    /* update modified time-stamp */
    type->modified_at = util_now_tsec();

    if (ti_task_add_mod_type_mod(task, field))
    {
        ex_set_mem(e);
        return;
    }
}

static void type__ren(
        ti_query_t * query,
        ti_type_t * type,
        ti_name_t * name,
        cleri_node_t * nd,
        ex_t * e)
{
    static const char * fnname = "mod_type` with task `ren";
    const int nargs = langdef_nd_n_function_params(nd);
    ti_field_t * field = ti_field_by_name(type, name);
    ti_task_t * task;
    ti_name_t * oldname;
    ti_raw_t * rname;

    if (fn_nargs(fnname, DOC_MOD_TYPE_REN, 4, nargs, e))
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
            fn_arg_str_slow(fnname, DOC_MOD_TYPE_REN, 4, query->rval, e))
        return;

    if (ti_opr_eq((ti_val_t *) field->name, query->rval))
        return;  /* do nothing, name is equal to current name */

    rname = (ti_raw_t *) query->rval;

    oldname = field->name;
    ti_incref(oldname);

    if (ti_field_set_name(field, (const char *) rname->data, rname->n, e))
        goto done;

    task = ti_task_get_task(query->ev, query->collection->root, e);
    if (!task)
        goto done;

    /* update modified time-stamp */
    type->modified_at = util_now_tsec();

    if (ti_task_add_mod_type_ren(task, field, oldname))
        ex_set_mem(e);

done:
    ti_name_drop(oldname);
}

static int do__f_mod_type(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    ti_type_t * type;
    ti_name_t * name;
    ti_raw_t * rmod;
    cleri_children_t * child = nd->children;
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_not_collection_scope("mod_type", query, e) ||
        fn_nargs_min("mod_type", DOC_MOD_TYPE, 3, nargs, e) ||
        ti_do_statement(query, child->node, e) ||
        fn_arg_str_slow("mod_type", DOC_MOD_TYPE, 1, query->rval, e))
        return e->nr;

    type = ti_types_by_raw(query->collection->types, (ti_raw_t *) query->rval);
    if (!type)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "type", e);

    if (ti_type_try_lock(type, e))
        return e->nr;

    ti_val_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, (child = child->next->next)->node, e) ||
        fn_arg_name_check("mod_type", DOC_MOD_TYPE, 2, query->rval, e))
        goto fail0;

    rmod = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, (child = child->next->next)->node, e) ||
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

    if (ti_raw_eq_strn(rmod, "ren", 3))
    {
        type__ren(query, type, name, nd, e);
        goto done;
    }

    ex_set(e, EX_VALUE_ERROR,
            "function `mod_type` expects argument 2 to be "
            "`add`, `del`, `mod` or `ren` but got `%.*s` instead"DOC_MOD_TYPE,
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
