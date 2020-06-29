#include <ti/fn/fn.h>

static inline int modtype__is_locked_cb(ti_thing_t * thing, ti_type_t * type)
{
    return thing->type_id == type->type_id && (thing->flags & TI_VFLAG_LOCK);
}

static inline int modtype__unlocked_cb(ti_thing_t * thing, void * UNUSED(arg))
{
    thing->flags &= ~TI_VFLAG_LOCK;
    return 0;
}

typedef struct
{
    ti_name_t * name;
    ti_val_t * dval;
    ti_type_t * type;
    ex_t * e;
} modtype__addv_t;

static inline int modtype__addv_cb(ti_thing_t * thing, modtype__addv_t * w)
{
    if (thing->type_id == w->type->type_id)
    {
        if (ti_val_make_assignable(&w->dval, thing, w->name, w->e))
            return w->e->nr;

        if (vec_push(&thing->items, w->dval))
        {
            ex_set_mem(w->e);
            return w->e->nr;
        }

        ti_incref(w->dval);
    }
    return 0;
}

static inline int modtype__delv_cb(ti_thing_t * thing, ti_field_t * field)
{
    if (thing->type_id == field->type->type_id)
        ti_val_drop(vec_swap_remove(thing->items, field->idx));
    return 0;
}

typedef struct
{
    ti_type_t * type;
    imap_t * imap;
} modtype__collect_t;

static int modtype__collect_cb(ti_thing_t * thing, modtype__collect_t * w)
{
    if (thing->type_id == w->type->type_id)
    {
        if (imap_add(w->imap, ti_thing_key(thing), thing))
            return -1;

        ti_incref(thing);
    }
    return 0;
}

typedef struct
{
    ti_field_t * field;
    ti_closure_t * closure;
    ti_query_t * query;
    ex_t * e;
} modtype__add_t;

static int modtype__add_cb(ti_thing_t * thing, modtype__add_t * w)
{
    ti_task_t * task;
    ti_prop_t * prop;
    ex_t ex = {0};

    assert (thing->type_id == w->field->type->type_id);

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
                    "field `%s` is added to type `%s` but at least one "
                    "error has occurred using the given callback; %s",
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

typedef struct
{
    ti_field_t * field;
    ti_closure_t * closure;
    ti_query_t * query;
    ti_val_t * dval;
    ex_t * e;
} modtype__mod_t;

static int modtype__mod_cb(ti_thing_t * thing, modtype__mod_t * w)
{
    ti_task_t * task;
    ti_prop_t * prop;
    ex_t ex = {0};

    assert (thing->type_id == w->field->type->type_id);

    if (w->closure->vars->n)
    {
        ti_incref(thing);
        prop = vec_get(w->closure->vars, 0);
        ti_val_drop(prop->val);
        prop->val = (ti_val_t *) thing;
    }

    if (ti_closure_do_statement(w->closure, w->query, &ex) ||
        ti_val_is_nil(w->query->rval) ||
        ti_field_make_assignable(w->field, &w->query->rval, thing, &ex))
    {
        ti_val_t * val = vec_get(thing->items, w->field->idx);

        if (w->e->nr == 0 && ex.nr)
        {
            ex_set(w->e, EX_OPERATION_ERROR,
                    "field `%s` on type `%s` is modified but at least one "
                    "error has occurred using the given callback; %s",
                    w->field->name->str,
                    w->field->type->name,
                    ex.msg);
        }

        /* the return value will not be used */
        ti_val_drop(w->query->rval);

        /*
         * no copy is required if the value has only one reference, therefore
         * it is safe to use `ti_field_make_assignable(..)` and this also
         * ensures that not much work has to be done since no copy of a set
         * or list will be made.
         */
        if (ti_field_make_assignable(w->field, &val, thing, &ex))
        {
            ti_incref(w->dval);
            ti_val_drop(vec_set(thing->items, w->dval, w->field->idx));

            if (thing->id)
            {
                task = ti_task_get_task(w->query->ev, thing, w->e);
                if (task && ti_task_add_set(task, w->field->name, w->dval))
                    ex_set_mem(w->e);
            }

            if (w->e->nr == 0 && ex.nr)
            {
                ex_set(w->e, EX_OPERATION_ERROR,
                        "field `%s` on type `%s` is modified but at least "
                        "one failed attempt was made to keep the original "
                        "value; %s",
                        w->field->name->str,
                        w->field->type->name,
                        ex.msg);
            }
        }
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

    /* none of the affected things may have a lock, (checked beforehand) */
    assert (~thing->flags & TI_VFLAG_LOCK);

    /* lock each thing so this value is final while modifying this type */
    thing->flags |= TI_VFLAG_LOCK;

    w->query->rval = NULL;
    return 0;
}

static int modtype__mod_after_cb(ti_thing_t * thing, modtype__mod_t * w)
{
    ti_task_t * task;
    ex_t ex = {0};
    ti_val_t * val = vec_get(thing->items, w->field->idx);

    assert (thing->type_id == w->field->type->type_id);

    /*
     * no copy is required if the value has only one reference, therefore
     * it is safe to use `ti_field_make_assignable(..)` and this also
     * ensures that not much work has to be done since no copy of a set
     * or list will be made.
     */
    if (ti_field_make_assignable(w->field, &val, thing, &ex))
    {
        ti_incref(w->dval);
        ti_val_drop(vec_set(thing->items, w->dval, w->field->idx));

        if (thing->id)
        {
            task = ti_task_get_task(w->query->ev, thing, w->e);
            if (task && ti_task_add_set(task, w->field->name, w->dval))
                ex_set_mem(w->e);
        }

        if (w->e->nr == 0 && ex.nr)
        {
            ex_set(w->e, EX_OPERATION_ERROR,
                    "field `%s` on type `%s` is modified but at least one new "
                    "instance was made with an inappropriate value which in "
                    "response is changed to default by ThingsDB; %s",
                    w->field->name->str,
                    w->field->type->name,
                    ex.msg);
        }
    }

    return 0;
}

static imap_t * modtype__collect_things(ti_query_t * query, ti_type_t * type)
{
    modtype__collect_t collect = {
            .imap = imap_create(),
            .type = type,
    };

    if (!collect.imap)
        return NULL;

    if (ti_query_vars_walk(
            query->vars,
            (imap_cb) modtype__collect_cb,
            &collect) ||
        imap_walk(
                query->collection->things,
                (imap_cb) modtype__collect_cb,
                &collect))
    {
        imap_destroy(collect.imap, (imap_destroy_cb) ti_val_drop);
        return NULL;
    }

    return collect.imap;
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
    ti_method_t * method = field ? NULL : ti_method_by_name(type, name);
    ti_closure_t * closure;
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_nargs_range(fnname, DOC_MOD_TYPE_ADD, 4, 5, nargs, e))
        return;

    if (field || method)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "property or method `%s` already exists on type `%s`",
                name->str, type->name);
        return;
    }

    child = nd->children->next->next->next->next->next->next;

    if (ti_do_statement(query, child->node, e))
        return;

    if (ti_val_is_closure(query->rval))
    {
        ti_closure_t * closure = (ti_closure_t *) query->rval;

        if (nargs == 5)
        {
            ex_set(e, EX_NUM_ARGUMENTS,
                    "function `%s` takes at most 4 arguments when used to "
                    "add a new method"DOC_MOD_TYPE_ADD,
                    fnname);
            return;
        }

        if (ti_closure_unbound(closure, e))
            return;

        task = ti_task_get_task(query->ev, query->collection->root, e);
        if (!task)
        {
            ex_set_mem(e);
            return;
        }

        if (ti_type_add_method(type, name, closure, e))
            return;

        if (ti_task_add_mod_type_add_method(task, type))
        {
            ti_type_remove_method(type, name);
            ex_set_mem(e);
        }

        return;
    }
    else if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `%s` expects argument 4 to be of "
            "type `"TI_VAL_STR_S"` or type `"TI_VAL_CLOSURE_S"` "
            "but got type `%s` instead"DOC_MOD_TYPE_ADD,
            fnname, ti_val_str(query->rval));
        return;
    }

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

    /*
     * Create the task at this point so the order of tasks is correct;
     * Must be after all `statements` with the old definition, but before
     * running statements with the new definition.
     */
    task = ti_task_get_task(query->ev, query->collection->root, e);
    if (!task)
        goto fail2;

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

    if (ti_task_add_mod_type_add_field(task, type, dval))
    {
        ex_set_mem(e);
        goto fail3;
    }

    modtype__addv_t addvjob = {
            .name = field->name,
            .dval = dval,
            .type = type,
            .e = e,
    };

    if (ti_query_vars_walk(query->vars, (imap_cb) modtype__addv_cb, &addvjob))
    {
        if (!e->nr)
            ex_set_mem(e);
        goto panic;
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
    dval = NULL;

    if (closure)
    {
        imap_t * imap;
        modtype__add_t addjob = {
                .field = field,
                .closure = closure,
                .query = query,
                .e = e,
        };

        imap = modtype__collect_things(query, type);
        if (!imap)
        {
            ex_set_mem(e);
            goto panic;
        }

        /*
         * Remove the closure from the query; The closure will be dropped
         * at the end of this block;
         */
        query->rval = NULL;

        /* cleanup before using the callback */
        ti_type_map_cleanup(field->type);

        (void) imap_walk(
                imap,
                (imap_cb) modtype__add_cb,
                &addjob);

        imap_destroy(imap, (imap_destroy_cb) ti_val_drop);

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
    ti_method_t * method = field ? NULL : ti_method_by_name(type, name);
    ti_task_t * task;

    if (fn_nargs(fnname, DOC_MOD_TYPE_DEL, 3, nargs, e))
        return;

    if (!field && !method)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no property or method `%s`",
                type->name, name->str);
        return;
    }

    task = ti_task_get_task(query->ev, query->collection->root, e);
    if (!task)
        return;

    if (method)
    {
        ti_type_remove_method(type, name);
        goto done;
    }

    /* check for variable to update, val_cache is not required since only
     * things with an id are store in cache
     */
    if (ti_query_vars_walk(
            query->vars,
            (imap_cb) modtype__delv_cb,
            field))
    {
        ex_set_mem(e);
        return;
    }

    if (ti_field_del(field, query->ev->id))
    {
        ex_set_mem(e);
        return;
    }

done:
    /* update modified time-stamp */
    type->modified_at = util_now_tsec();

    if (ti_task_add_mod_type_del(task, type, name))
        ex_set_mem(e);
}

static int type__mod_using_callback(
        const char * fnname,
        ti_query_t * query,
        cleri_node_t * nd,
        ti_field_t * field,
        ti_raw_t * spec_raw,
        ex_t * e)
{
    ti_closure_t * closure;
    imap_t * imap, * imap_after;
    ti_task_t * task;

    if (ti_do_statement(query, nd, e) ||
        fn_arg_closure(fnname, DOC_MOD_TYPE_MOD, 5, query->rval, e))
        return e->nr;

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (ti_closure_try_wse(closure, query, e) ||
        ti_closure_inc(closure, query, e))
        goto fail0;

    modtype__mod_t modjob = {
            .field = ti_field_as_new(field, spec_raw, e),
            .closure = closure,
            .query = query,
            .dval = NULL,
            .e = e,
    };

    if (!modjob.field)
        goto fail1;  /* error is set */

    modjob.dval = ti_field_dval(modjob.field);
    if (!modjob.dval)
        goto fail2;  /* error is set */

    imap = modtype__collect_things(query, field->type);
    if (!imap)
    {
        ex_set_mem(e);
        goto fail3;
    }

    task = ti_task_get_task(query->ev, query->collection->root, e);
    if (!task)
        goto fail4;

    if (ti_field_mod(
            field,
            (ti_raw_t *) ti_val_borrow_any_str(),
            query->vars,
            e))
        goto fail4;

    /* From now on it is critical and we should panic */

    /* update modified time-stamp */
    field->type->modified_at = util_now_tsec();

    /* add a modify to any */
    if (ti_task_add_mod_type_mod_field(task, field))
    {
        ex_set_mem(e);
        goto panic;
    }

    /* cleanup before using the callback */
    ti_type_map_cleanup(field->type);

    (void) imap_walk(
            imap,
            (imap_cb) modtype__mod_cb,
            &modjob);

    (void) imap_walk(
            imap,
            (imap_cb) modtype__unlocked_cb,
            NULL);

    imap_after = modtype__collect_things(query, field->type);
    if (imap_after)
    {
        imap_difference_inplace(imap_after, imap);

        (void) imap_walk(
                imap_after,
                (imap_cb) modtype__mod_after_cb,
                &modjob);

        imap_destroy(imap_after, (imap_destroy_cb) ti_val_drop);
    }

    /* get a new task since the order of the task must be after the changes
     * above */
    task = ti_task_get_task(query->ev, query->collection->root, e);
    if (!task)
        goto panic;

    if (ti_task_add_mod_type_mod_field(task, modjob.field))
    {
        ex_set_mem(e);
        goto panic;
    }

    ti_field_replace(field, &modjob.field);
fail4:
    imap_destroy(imap, (imap_destroy_cb) ti_val_drop);
fail3:
    ti_val_drop(modjob.dval);
fail2:
    ti_field_destroy_dep(modjob.field);  /* modjob.field may be NULL */
fail1:
    ti_closure_dec(closure, query);
fail0:
    ti_val_drop((ti_val_t *) closure);
    return e->nr;
panic:
    ti_panic("unrecoverable state after using mod_type(...)");
    goto fail4;
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
    ti_method_t * method = field ? NULL : ti_method_by_name(type, name);

    cleri_children_t * child;

    if (fn_nargs_range(fnname, DOC_MOD_TYPE_MOD, 4, 5, nargs, e))
        return;

    if (!field && !method)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no property or method `%s`",
                type->name, name->str);
        return;
    }

    child = nd->children->next->next->next->next->next->next;

    if (ti_do_statement(query, child->node, e))
        return;

    if (ti_val_is_closure(query->rval))
    {
        ti_task_t * task;
        ti_closure_t * closure = (ti_closure_t *) query->rval;

        if (nargs == 5)
        {
            ex_set(e, EX_NUM_ARGUMENTS,
                    "function `%s` takes at most 4 arguments when used to "
                    "modify a method"DOC_MOD_TYPE_MOD,
                    fnname);
            return;
        }

        if (!method)
        {
            ex_set(e, EX_TYPE_ERROR,
                "cannot convert a property into a method"DOC_MOD_TYPE_MOD,
                fnname, ti_val_str(query->rval));
            return;
        }

        if (ti_closure_unbound(closure, e))
            return;

        task = ti_task_get_task(query->ev, query->collection->root, e);
        if (!task)
        {
            ex_set_mem(e);
            return;
        }

        ti_val_drop((ti_val_t *) method->closure);
        method->closure = closure;

        ti_incref(closure);

        if (ti_type_add_method(type, name, closure, e))
            return;

        if (ti_task_add_mod_type_mod_method(task, type, method))
        {
            ti_type_remove_method(type, name);
            ex_set_mem(e);
        }

        return;
    }

    if (ti_val_is_str(query->rval))
    {
        ti_raw_t * spec_raw;

        if (!field)
        {
            ex_set(e, EX_TYPE_ERROR,
                "cannot convert a method into a property"DOC_MOD_TYPE_MOD,
                fnname, ti_val_str(query->rval));
            return;
        }
        /* continue modifying a field */

        spec_raw = (ti_raw_t *) query->rval;
        query->rval = NULL;

        if (nargs == 4)
        {
            ti_task_t * task;

            if (ti_field_mod(field, spec_raw, query->vars, e))
                goto fail;

            task = ti_task_get_task(query->ev, query->collection->root, e);
            if (!task)
                goto fail;

            /* update modified time-stamp */
            type->modified_at = util_now_tsec();

            if (ti_task_add_mod_type_mod_field(task, field))
            {
                ex_set_mem(e);
                goto fail;
            }
        }
        else
        {
            (void) type__mod_using_callback(
                    fnname,
                    query,
                    child->next->next->node,
                    field,
                    spec_raw,
                    e);
        }
fail:
        ti_val_drop((ti_val_t *) spec_raw);
        return;
    }

    ex_set(e, EX_TYPE_ERROR,
        "function `%s` expects argument 4 to be of "
        "type `"TI_VAL_STR_S"` or type `"TI_VAL_CLOSURE_S"` "
        "but got type `%s` instead"DOC_MOD_TYPE_MOD,
        fnname, ti_val_str(query->rval));
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
    ti_method_t * method = field ? NULL : ti_method_by_name(type, name);
    ti_task_t * task;
    ti_name_t * oldname;
    ti_name_t * newname;
    ti_raw_t * rname;

    if (fn_nargs(fnname, DOC_MOD_TYPE_REN, 4, nargs, e))
        return;

    if (!field && !method)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no property or method `%s`",
                type->name, name->str);
        return;
    }

    if (ti_do_statement(
            query,
            nd->children->next->next->next->next->next->next->node,
            e) ||
            fn_arg_str_slow(fnname, DOC_MOD_TYPE_REN, 4, query->rval, e))
        return;

    rname = (ti_raw_t *) query->rval;

    oldname = field ? field->name : method->name;
    ti_incref(oldname);

    /* method */
    if (ti_opr_eq((ti_val_t *) oldname, query->rval))
        goto done;  /* do nothing, name is equal to current name */

    if (method)
    {
        if (ti_method_set_name(
                method,
                type,
                (const char *) rname->data,
                rname->n,
                e))
            goto done;
        newname = method->name;
    }
    else
    {
        if (ti_field_set_name(
                field,
                (const char *) rname->data,
                rname->n,
                e))
            goto done;
        newname = field->name;
    }

    task = ti_task_get_task(query->ev, query->collection->root, e);
    if (!task)
        goto done;

    /* update modified time-stamp */
    type->modified_at = util_now_tsec();

    if (ti_task_add_mod_type_ren(task, type, oldname, newname))
        ex_set_mem(e);

done:
    ti_name_drop(oldname);
}

static int modtype__has_lock(ti_query_t * query, ti_type_t * type, ex_t * e)
{
    int rc = ti_query_vars_walk(
            query->vars,
            (imap_cb) modtype__is_locked_cb,
            type);

    if (rc)
    {
        if (rc < 0)
            goto memerror;
        goto locked;
    }

    if (imap_walk(
            query->collection->things,
            (imap_cb) modtype__is_locked_cb,
            type))
        goto locked;

    return 0;

locked:
    ex_set(e, EX_OPERATION_ERROR,
        "cannot change type `%s` while one of the instances is being used",
        type->name);
    return e->nr;
memerror:
    ex_set_mem(e);
    return e->nr;
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

    /* Locks may be acquired when reading one of the arguments; This is fine
     * since then they also are guaranteed to be released once the argument
     * is read. It is not a good state if one of the affected things has a
     * lock outside this function call.
     */
    if (modtype__has_lock(query, type, e) || ti_type_try_lock(type, e))
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
