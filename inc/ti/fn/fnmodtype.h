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
    ti_field_t * field;
    ti_val_t * dval;
    ti_type_t * type;
    ex_t * e;
} modtype__addv_t;

static inline int modtype__addv_cb(ti_thing_t * thing, modtype__addv_t * w)
{
    if (thing->type_id == w->type->type_id)
    {
        if (ti_val_make_assignable(&w->dval, thing, w->field, w->e))
            return w->e->nr;

        if (vec_push(&thing->items.vec, w->dval))
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
        ti_val_unsafe_drop(vec_swap_remove(thing->items.vec, field->idx));
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

    assert(thing->type_id == w->field->type->type_id);

    if (w->closure->vars->n)
    {
        ti_incref(thing);
        prop = VEC_get(w->closure->vars, 0);
        ti_val_unsafe_drop(prop->val);
        prop->val = (ti_val_t *) thing;
    }

    if (ti_closure_do_statement(w->closure, w->query, &ex) ||
        ti_val_is_nil(w->query->rval) ||
        w->query->rval == VEC_get(thing->items.vec, w->field->idx) ||
        ti_field_make_assignable(w->field, &w->query->rval, thing, &ex))
    {
        if (w->e->nr == 0 && ex.nr)
        {
            ex_set(w->e, EX_OPERATION,
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
        ti_val_unsafe_drop(vec_set(
                thing->items.vec,
                w->query->rval,
                w->field->idx));

        if (thing->id)
        {
            task = ti_task_get_task(w->query->change, thing);
            if (!task || ti_task_add_set(
                    task,
                    (ti_raw_t *) w->field->name,
                    w->query->rval))
                ex_set_mem(w->e);
        }
    }

    w->query->rval = NULL;
    return 0;
}

typedef struct
{
    ti_field_t * field;
    ti_field_t * true_field;
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

    assert(thing->type_id == w->field->type->type_id);

    if (w->closure->vars->n)
    {
        ti_incref(thing);
        prop = VEC_get(w->closure->vars, 0);
        ti_val_unsafe_drop(prop->val);
        prop->val = (ti_val_t *) thing;
    }

    if (ti_closure_do_statement(w->closure, w->query, &ex) ||
        ti_val_is_nil(w->query->rval) ||
        ti_field_make_assignable(w->field, &w->query->rval, thing, &ex))
    {
        ti_val_t * val = VEC_get(thing->items.vec, w->field->idx);
        if (w->e->nr == 0 && ex.nr)
        {
            ex_set(w->e, EX_OPERATION,
                    "field `%s` on type `%s` is modified but at least one "
                    "error has occurred using the given callback: %s",
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
        if (ti_field_make_assignable(w->field, &val, thing, &ex) &&
            !ti_val_make_assignable(&w->dval, thing, w->field, w->e))
        {
            ti_incref(w->dval);
            ti_val_unsafe_drop(vec_set(
                    thing->items.vec,
                    w->dval,
                    w->field->idx));

            if (thing->id)
            {
                task = ti_task_get_task(w->query->change, thing);
                if (!task || ti_task_add_set(
                        task,
                        (ti_raw_t *) w->field->name,
                        w->dval))
                    ex_set_mem(w->e);
            }

            if (w->e->nr == 0 && ex.nr)
            {
                ex_set(w->e, EX_OPERATION,
                        "field `%s` on type `%s` is modified but at least "
                        "one failed attempt was made to keep the original "
                        "value: %s",
                        w->field->name->str,
                        w->field->type->name,
                        ex.msg);
            }
        }
    }
    else
    {
        ti_val_unsafe_drop(vec_set(
                thing->items.vec,
                w->query->rval,
                w->field->idx));

        if (thing->id)
        {
            task = ti_task_get_task(w->query->change, thing);
            if (!task || ti_task_add_set(
                    task,
                    (ti_raw_t *) w->field->name,
                    w->query->rval))
                ex_set_mem(w->e);
        }
    }

    if (ti_spec_is_arr_or_set(w->field->spec))
    {
        ti_varr_t * varr_or_vset = VEC_get(thing->items.vec, w->field->idx);
        varr_or_vset->key_ = w->true_field;
    }

    /* none of the affected things may have a lock, (checked beforehand) */
    assert(~thing->flags & TI_VFLAG_LOCK);

    /* lock each thing so this value is final while modifying this type */
    thing->flags |= TI_VFLAG_LOCK;

    w->query->rval = NULL;
    return 0;
}

static int modtype__mod_after_cb(ti_thing_t * thing, modtype__mod_t * w)
{
    ti_task_t * task;
    ex_t ex = {0};
    ti_val_t * val = VEC_get(thing->items.vec, w->field->idx);

    assert(thing->type_id == w->field->type->type_id);

    /*
     * no copy is required if the value has only one reference, therefore
     * it is safe to use `ti_field_make_assignable(..)` and this also
     * ensures that not much work has to be done since no copy of a set
     * or list will be made.
     */
    if (ti_field_make_assignable(w->field, &val, thing, &ex) &&
        !ti_val_make_assignable(&w->dval, thing, w->field, w->e))
    {
        ti_incref(w->dval);
        ti_val_unsafe_drop(vec_set(thing->items.vec, w->dval, w->field->idx));

        if (thing->id)
        {
            task = ti_task_get_task(w->query->change, thing);
            if (!task || ti_task_add_set(
                    task,
                    (ti_raw_t *) w->field->name,
                    w->dval))
                ex_set_mem(w->e);
        }

        if (w->e->nr == 0 && ex.nr)
        {
            ex_set(w->e, EX_OPERATION,
                    "field `%s` on type `%s` is modified but at least one "
                    "instance got an inappropriate value from the migration "
                    "callback; to be compliant, ThingsDB has used the default "
                    "value for this instance; callback response: %s",
                    w->field->name->str,
                    w->field->type->name,
                    ex.msg);
        }
    }

    return 0;
}

typedef struct
{
    ti_closure_t * closure;
    ti_query_t * query;
    ex_t * e;
} modtype__all_t;

static int modtype__all_cb(ti_thing_t * thing, modtype__all_t * w)
{
    if (w->closure->vars->n)
    {
        ti_prop_t * prop = VEC_get(w->closure->vars, 0);
        ti_incref(thing);
        ti_val_unsafe_drop(prop->val);
        prop->val = (ti_val_t *) thing;
    }

    (void) ti_closure_do_statement(w->closure, w->query, w->e);

    ti_val_drop(w->query->rval);
    w->query->rval = NULL;

    return w->e->nr;
}

static void type__add(
        ti_query_t * query,
        ti_type_t * type,
        ti_name_t * name,
        cleri_node_t * nd,
        ex_t * e)
{
    static const char * fnname = "mod_type` with task `add";
    cleri_node_t * child;
    ti_task_t * task;
    ti_raw_t * spec_raw;
    ti_val_t * dval;
    ti_field_t * field = ti_field_by_name(type, name);
    ti_method_t * method = field ? NULL : ti_type_get_method(type, name);
    ti_closure_t * closure;
    const int nargs = fn_get_nargs(nd);

    if (fn_nargs_range(fnname, DOC_MOD_TYPE_ADD, 4, 5, nargs, e))
        return;

    if (type->idname == name || field || method)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "property or method `%s` already exists on type `%s`",
                name->str, type->name);
        return;
    }

    child = nd->children->next->next->next->next->next->next;

    if (ti_do_statement(query, child, e))
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

        task = ti_task_get_task(query->change, query->collection->root);
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

    spec_raw = ti_type_nested_from_val(type, query->rval, e);
    if (e->nr)
        return;

    if (spec_raw)
        ti_val_unsafe_gc_drop(query->rval);
    else
    {
        if (!ti_val_is_str(query->rval))
        {
            ex_set(e, EX_TYPE_ERROR,
                "function `%s` expects argument 4 to be of "
                "type `"TI_VAL_STR_S"` or type `"TI_VAL_CLOSURE_S"` "
                "but got type `%s` instead"DOC_MOD_TYPE_ADD,
                fnname, ti_val_str(query->rval));
            return;
        }
        spec_raw = (ti_raw_t *) query->rval;

        if (spec_raw->n == 1 && spec_raw->data[0] == '#')
        {
            if (nargs == 5)
            {
                ex_set(e, EX_NUM_ARGUMENTS,
                        "function `%s` takes at most 4 arguments when adding "
                        "an Id ('#') definition"DOC_MOD_TYPE_ADD,
                        fnname);
                return;
            }

            if (type->idname)
            {
                ex_set(e, EX_LOOKUP_ERROR,
                        "multiple Id ('#') definitions on type `%s`",
                        type->name);
                return;
            }

            task = ti_task_get_task(query->change, query->collection->root);
            if (!task)
            {
                ex_set_mem(e);
                return;
            }

            /* update modified time-stamp */
            type->modified_at = util_now_usec();
            type->idname = name;
            ti_incref(name);

            if (ti_task_add_mod_type_add_idname(task, type))
            {
                /* modified is wrong; the rest we can revert */
                type->idname = NULL;
                ti_decref(name);
                ex_set_mem(e);
            }
            return;
        }
    }

    query->rval = NULL;

    if (nargs == 5)
    {
        if (ti_type_is_wrap_only(type))
        {
            ex_set(e, EX_NUM_ARGUMENTS,
                    "function `%s` takes at most 4 arguments when used on "
                    "a type with wrap-only mode enabled"DOC_MOD_TYPE_ADD,
                    fnname);
            goto fail0;
        }
        /*
         * Statements must be parsed before creating a new field since
         * potentially generated instances of this type must be without the
         * new field.
         */
        if (ti_do_statement(query, child->next->next, e))
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
    else if (!ti_type_is_wrap_only(type))
    {
        dval = field->dval_cb(field);
        if (!dval)
        {
            ex_set_mem(e);
            goto fail1;
        }
    }
    else
    {
        dval = NULL;
    }

    /*
     * Create the task at this point so the order of tasks is correct;
     * Must be after all `statements` with the old definition, but before
     * running statements with the new definition.
     */
    task = ti_task_get_task(query->change, query->collection->root);
    if (!task)
    {
        ex_set_mem(e);
        goto fail2;
    }

    closure = (ti_closure_t *) query->rval;

    if (closure)
    {
        /* we must have a default value when having a closure */
        assert(dval);

        if (ti_closure_try_wse(closure, query, e) ||
            ti_closure_inc(closure, query, e))
            goto fail2;
    }

    /* here we create the ID's for optional new things */
    if (dval && ti_val_gen_ids(dval))
    {
        ex_set_mem(e);
        goto fail3;
    }

    /* update modified time-stamp */
    type->modified_at = util_now_usec();

    if (ti_task_add_mod_type_add_field(task, type, dval))
    {
        ex_set_mem(e);
        goto fail3;
    }

    if (ti_type_is_wrap_only(type))
    {
        /* we are finished when we do not have a default value to set */
        assert(dval == NULL);
        return;
    }

    modtype__addv_t addvjob = {
            .field = field,
            .dval = dval,
            .type = type,
            .e = e,
    };

    if (ti_query_vars_walk(
            query->vars,
            query->collection,
            (imap_cb) modtype__addv_cb,
            &addvjob))
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
    if (ti_field_init_things(field, &dval))
    {
        ex_set_mem(e);
        goto panic;
    }

    ti_val_unsafe_drop(dval);
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

        imap = ti_type_collect_things(query, type);
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

        imap_destroy(imap, (imap_destroy_cb) ti_val_unsafe_drop);

        ti_closure_dec(closure, query);
        ti_val_unsafe_drop((ti_val_t *) closure);
    }

    return;

panic:
    ti_panic("unrecoverable state after using mod_type(...)");

fail3:
    ti_closure_dec(closure, query);

fail2:
    ti_val_unsafe_drop(dval);

fail1:
    assert(e->nr);
    ti_field_remove(field);
    return;  /* failed */

fail0:
    ti_val_unsafe_drop((ti_val_t *) spec_raw);
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
    const int nargs = fn_get_nargs(nd);
    ti_field_t * field = ti_field_by_name(type, name);
    ti_method_t * method = field ? NULL : ti_type_get_method(type, name);
    ti_task_t * task;

    if (fn_nargs(fnname, DOC_MOD_TYPE_DEL, 3, nargs, e))
        return;

    if (type->idname != name && !field && !method)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no property or method `%s`",
                type->name, name->str);
        return;
    }

    if (field && ti_field_has_relation(field))
    {
        ex_set(e, EX_TYPE_ERROR,
                "cannot delete a property with a relation; "
                "you might want to remove the relation by using: "
                "`mod_type(\"%s\", \"rel\", \"%s\", nil);`",
                type->name, name->str);
        return;
    }

    task = ti_task_get_task(query->change, query->collection->root);
    if (!task)
    {
        ex_set_mem(e);
        return;
    }

    if (type->idname == name)
    {
        ti_name_unsafe_drop(type->idname);
        type->idname = NULL;
        goto done;
    }

    if (method)
    {
        ti_type_remove_method(type, name);
        goto done;
    }

    /* check for variable to update */
    if (ti_query_vars_walk(
            query->vars,
            query->collection,
            (imap_cb) modtype__delv_cb,
            field))
    {
        ex_set_mem(e);
        return;
    }

    if (ti_field_del(field))
    {
        ex_set_mem(e);
        return;
    }

done:
    /* update modified time-stamp */
    type->modified_at = util_now_usec();

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
            .true_field = field,
            .closure = closure,
            .query = query,
            .dval = NULL,
            .e = e,
    };

    if (!modjob.field)
        goto fail1;  /* error is set */

    modjob.dval = modjob.field->dval_cb(modjob.field);
    if (!modjob.dval)
        goto fail2;  /* error is set */

    imap = ti_type_collect_things(query, field->type);
    if (!imap)
    {
        ex_set_mem(e);
        goto fail3;
    }

    task = ti_task_get_task(query->change, query->collection->root);
    if (!task)
    {
        ex_set_mem(e);
        goto fail4;
    }

    if (ti_field_mod(field, (ti_raw_t *) ti_val_borrow_any_str(), e))
        goto fail4;

    /* From now on it is critical and we should panic */

    /* update modified time-stamp */
    field->type->modified_at = util_now_usec();

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

    imap_after = ti_type_collect_things(query, field->type);
    if (imap_after)
    {
        imap_difference_inplace(imap_after, imap);

        (void) imap_walk(
                imap_after,
                (imap_cb) modtype__mod_after_cb,
                &modjob);

        imap_destroy(imap_after, (imap_destroy_cb) ti_val_unsafe_drop);
    }

    /* get a new task since the order of the task must be after the changes
     * above */
    task = ti_task_get_task(query->change, query->collection->root);
    if (!task || ti_task_add_mod_type_mod_field(task, modjob.field))
    {
        ex_set_mem(e);
        goto panic;
    }

    ti_field_replace(field, &modjob.field);
fail4:
    imap_destroy(imap, (imap_destroy_cb) ti_val_unsafe_drop);
fail3:
    ti_val_unsafe_drop(modjob.dval);
fail2:
    ti_field_destroy_dep(modjob.field);  /* modjob.field may be NULL */
fail1:
    ti_closure_dec(closure, query);
fail0:
    ti_val_unsafe_drop((ti_val_t *) closure);
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
    const int nargs = fn_get_nargs(nd);
    ti_field_t * field = ti_field_by_name(type, name);
    ti_method_t * method = field ? NULL : ti_type_get_method(type, name);
    ti_raw_t * spec_raw = NULL;
    cleri_node_t * child;

    if (fn_nargs_range(fnname, DOC_MOD_TYPE_MOD, 4, 5, nargs, e))
        return;

    if (type->idname == name)
    {
        ex_set(e, EX_TYPE_ERROR,
                "cannot modify an Id ('#') definition");
        return;
    }

    if (!field && !method)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no property or method `%s`",
                type->name, name->str);
        return;
    }

    if (field && ti_field_has_relation(field))
    {
        ex_set(e, EX_TYPE_ERROR,
                "cannot modify a property with a relation; "
                "you might want to remove the relation by using: "
                "`mod_type(\"%s\", \"rel\", \"%s\", nil);`",
                type->name, name->str);
        return;
    }

    child = nd->children->next->next->next->next->next->next;

    if (ti_do_statement(query, child, e))
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
                "cannot convert a property into a method"DOC_MOD_TYPE_MOD);
            return;
        }

        if (ti_closure_unbound(closure, e))
            return;

        task = ti_task_get_task(query->change, query->collection->root);
        if (!task)
        {
            ex_set_mem(e);
            return;
        }

        ti_method_set_closure(method, closure);

        if (ti_task_add_mod_type_mod_method(task, type, method))
        {
            ti_type_remove_method(type, name);
            ex_set_mem(e);
        }

        return;
    }

    spec_raw = ti_type_nested_from_val(type, query->rval, e);
    if (e->nr)
        return;

    if (ti_val_is_str(query->rval))
    {
        if (!field)
        {
            ex_set(e, EX_TYPE_ERROR,
                "cannot convert a method into a property"DOC_MOD_TYPE_MOD);
            return;
        }
        /* continue modifying a field */

        spec_raw = (ti_raw_t *) query->rval;
        if (spec_raw->n == 1 && spec_raw->data[0] == '#')
        {
            ex_set(e, EX_TYPE_ERROR,
                "cannot convert a property into an Id ('#') definition");
            return;
        }

        query->rval = NULL;
    }

    if (spec_raw)
    {
        if (nargs == 4)
        {
            ti_task_t * task;

            if (ti_field_mod(field, spec_raw, e))
                goto fail;

            task = ti_task_get_task(query->change, query->collection->root);
            if (!task)
            {
                ex_set_mem(e);
                goto fail;
            }

            /* update modified time-stamp */
            type->modified_at = util_now_usec();

            if (ti_task_add_mod_type_mod_field(task, field))
            {
                ex_set_mem(e);
                goto fail;
            }
        }
        else
        {
            if (ti_type_is_wrap_only(type))
            {
                ex_set(e, EX_NUM_ARGUMENTS,
                        "function `%s` takes at most 4 arguments when used on "
                        "a type with wrap-only mode enabled"DOC_MOD_TYPE_MOD,
                        fnname);
                goto fail;
            }

            (void) type__mod_using_callback(
                    fnname,
                    query,
                    child->next->next,
                    field,
                    spec_raw,
                    e);
        }
fail:
        ti_val_unsafe_drop((ti_val_t *) spec_raw);
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
    const int nargs = fn_get_nargs(nd);
    ti_field_t * field = ti_field_by_name(type, name);
    ti_method_t * method = field ? NULL : ti_type_get_method(type, name);
    ti_task_t * task;
    ti_name_t * oldname;
    ti_name_t * newname;
    ti_raw_t * rname;

    if (fn_nargs(fnname, DOC_MOD_TYPE_REN, 4, nargs, e))
        return;

    if (type->idname != name && !field && !method)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no property or method `%s`",
                type->name, name->str);
        return;
    }

    if (ti_do_statement(
            query,
            nd->children->next->next->next->next->next->next,
            e) ||
            fn_arg_str_slow(fnname, DOC_MOD_TYPE_REN, 4, query->rval, e))
        return;

    rname = (ti_raw_t *) query->rval;

    oldname = field
            ? field->name
            : method
            ? method->name
            : type->idname;
    ti_incref(oldname);

    /* method */
    if (ti_opr_eq((ti_val_t *) oldname, query->rval))
        goto done;  /* do nothing, name is equal to current name */

    if (field)
    {
        if (ti_field_set_name(
                field,
                (const char *) rname->data,
                rname->n,
                e))
            goto done;
        newname = field->name;
    }
    else if (method)
    {
        if (ti_method_set_name_t(
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
        if (ti_type_set_idname(
                type,
                (const char *) rname->data,
                rname->n,
                e))
            goto done;
        newname = type->idname;
    }

    task = ti_task_get_task(query->change, query->collection->root);
    if (!task)
    {
        ex_set_mem(e);
        goto done;
    }

    /* update modified time-stamp */
    type->modified_at = util_now_usec();

    if (ti_task_add_mod_type_ren(task, type, oldname, newname))
        ex_set_mem(e);

done:
    ti_name_unsafe_drop(oldname);
}

static int modtype__has_lock(ti_query_t * query, ti_type_t * type, ex_t * e)
{
    int rc = ti_query_vars_walk(
            query->vars,
            query->collection,
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
            type) ||
        ti_gc_walk(
            query->collection->gc,
            (queue_cb) modtype__is_locked_cb,
            type))
        goto locked;

    return 0;

locked:
    ex_set(e, EX_OPERATION,
        "cannot change type `%s` while one of the instances is in use",
        type->name);
    return e->nr;
memerror:
    ex_set_mem(e);
    return e->nr;
}

static ti_type_t * modtype__type_from_field(ti_field_t * field, ex_t * e)
{
    uint16_t spec = field->spec;

    if (spec == TI_SPEC_SET)
        spec = field->nested_spec | TI_SPEC_NILLABLE;

    if ((spec & TI_SPEC_MASK_NILLABLE) >= TI_SPEC_ANY ||
        (~spec & TI_SPEC_NILLABLE))
    {
        ex_set(e, EX_TYPE_ERROR,
                "relations may only be configured between restricted "
                "sets and/or nillable typed"DOC_MOD_TYPE_REL);
        return NULL;
    }

    if (field->condition.rel)
    {
        ex_set(e, EX_TYPE_ERROR,
                "relation for `%s` on type `%s` already exists",
                field->name->str, field->type->name);
        return NULL;
    }

    return ti_types_by_id(field->type->types, spec & TI_SPEC_MASK_NILLABLE);
}

static void type__rel_add(
        ti_query_t * query,
        ti_field_t * field,
        ti_name_t * name,
        ex_t * e)
{
    ti_type_t * type, * otype;
    ti_field_t * ofield;
    ti_task_t * task;

    otype = modtype__type_from_field(field, e);
    if (!otype)
        return;

    ofield = ti_field_by_name(otype, name);
    if (!ofield)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no property `%s`",
                otype->name, name->str);
        return;
    }

    type = modtype__type_from_field(ofield, e);
    if (!type)
        return;

    if (type != field->type)
    {
        ex_set(e, EX_TYPE_ERROR,
                "failed to create relation; "
                "property `%s` on type `%s` is referring to type `%s`",
                ofield->name->str, otype->name, type->name);
        return;
    }

    if (type != otype && (
        modtype__has_lock(query, otype, e) || ti_type_try_lock(otype, e)))
        return;

    if (ti_field_relation_check(field, ofield, query->vars, e))
        goto fail0;

    task = ti_task_get_task(query->change, query->collection->root);
    if (!task)
    {
        ex_set_mem(e);
        goto fail0;
    }

    if (ti_condition_field_rel_init(field, ofield, e))
        goto fail0;

    if (ti_field_relation_make(field, ofield, query->vars))
    {
        ti_panic("unrecoverable error");
        ex_set_mem(e);
        goto fail0;
    }

    /* update modified time-stamp */
    type->modified_at = util_now_usec();
    otype->modified_at = type->modified_at;

    if (ti_task_add_mod_type_rel_add(
            task,
            type,
            field->name,
            otype,
            ofield->name))
        ex_set_mem(e);

fail0:
    ti_type_unlock(otype, type != otype /* only when type are not equal*/);
}

static void type__rel_del(
        ti_query_t * query,
        ti_field_t * field,
        ex_t * e)
{
    ti_field_t * ofield;
    ti_task_t * task;

    if (!ti_field_has_relation(field))
    {
        ex_set(e, EX_TYPE_ERROR,
                "missing relation for `%s` on type `%s`",
                field->name->str, field->type->name);
        return;
    }

    ofield = field->condition.rel->field;

    if (field->type != ofield->type && (
        modtype__has_lock(query, ofield->type, e) ||
        ti_type_try_lock(ofield->type, e)))
        return;

    task = ti_task_get_task(query->change, query->collection->root);
    if (!task)
    {
        ex_set_mem(e);
        goto fail0;
    }

    /* update modified time-stamp */
    field->type->modified_at = util_now_usec();
    ofield->type->modified_at = field->type->modified_at;

    free(field->condition.rel);
    field->condition.rel = NULL;

    free(ofield->condition.rel);
    ofield->condition.rel = NULL;

    if (ti_task_add_mod_type_rel_del(
            task,
            field->type,
            field->name))
        ex_set_mem(e);

fail0:
    ti_type_unlock(ofield->type, field->type != ofield->type);
}

static void type__rel(
        ti_query_t * query,
        ti_type_t * type,
        ti_name_t * name,
        cleri_node_t * nd,
        ex_t * e)
{
    static const char * fnname = "mod_type` with task `rel";
    const int nargs = fn_get_nargs(nd);
    ti_field_t * field = ti_field_by_name(type, name);

    if (fn_nargs(fnname, DOC_MOD_TYPE_REL, 4, nargs, e))
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
            nd->children->next->next->next->next->next->next,
            e))
        return;

    if (ti_val_is_str(query->rval))
    {
        name = ti_names_from_raw((ti_raw_t *) query->rval);
        if (!name)
        {
            ex_set_mem(e);
            return;
        }
        type__rel_add(query, field, name, e);
        ti_name_drop(name);
        return;
    }

    if (ti_val_is_nil(query->rval))
    {
        type__rel_del(query, field, e);
        return;
    }

    ex_set(e, EX_TYPE_ERROR,
        "function `%s` expects argument 4 to be of "
        "type `"TI_VAL_STR_S"` or type `"TI_VAL_NIL_S"` "
        "but got type `%s` instead"DOC_MOD_TYPE_REL,
        fnname, ti_val_str(query->rval));
}

static void type__all(
        ti_query_t * query,
        ti_type_t * type,
        cleri_node_t * nd,
        ex_t * e)
{
    static const char * fnname = "mod_type` with task `all";
    const int nargs = fn_get_nargs(nd);

    ti_closure_t * closure;
    ti_task_t * task;
    imap_t * imap;

    nd = nd->children->next->next->next->next;

    if (fn_nargs(fnname, DOC_MOD_TYPE_ALL, 3, nargs, e) ||
        ti_do_statement(query, nd, e) ||
        fn_arg_closure(fnname, DOC_MOD_TYPE_ALL, 3, query->rval, e))
        return;

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (ti_closure_try_wse(closure, query, e) ||
        ti_closure_inc(closure, query, e))
        goto fail0;

    imap = ti_type_collect_things(query, type);
    if (!imap)
    {
        ex_set_mem(e);
        goto fail1;
    }

    task = ti_task_get_task(query->change, query->collection->root);
    if (!task)
    {
        ex_set_mem(e);
        goto fail2;
    }

    modtype__all_t w = {
            .query = query,
            .e = e,
            .closure = closure,
    };

    (void) imap_walk(imap, (imap_cb) modtype__all_cb, &w);

fail2:
    imap_destroy(imap, (imap_destroy_cb) ti_val_unsafe_drop);
fail1:
    ti_closure_dec(closure, query);
fail0:
    ti_val_unsafe_drop((ti_val_t *) closure);
    return;
}

static void type__wpo(
        ti_query_t * query,
        ti_type_t * type,
        cleri_node_t * nd,
        ex_t * e)
{
    static const char * fnname = "mod_type` with task `wpo";
    const int nargs = fn_get_nargs(nd);
    _Bool wrap_only;
    ti_task_t * task;
    ssize_t n;

    nd = nd->children->next->next->next->next;

    if (fn_nargs(fnname, DOC_MOD_TYPE_WPO, 3, nargs, e) ||
        ti_do_statement(query, nd, e) ||
        fn_arg_bool(fnname, DOC_MOD_TYPE_WPO, 3, query->rval, e))
        return;

    wrap_only = ti_val_as_bool(query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (wrap_only == ti_type_is_wrap_only(type))
        return;  /* nothing to do */

    if (wrap_only && (n = ti_query_count_type(query, type)))
    {
        if (n < 0)
            ex_set_mem(e);
        else
            ex_set(e, EX_OPERATION,
                "a type can only be changed to `wrap-only` mode without "
                "having active instances; "
                "%zd active instance%s of type `%s` %s been found"
                DOC_MOD_TYPE_WPO,
                n, n == 1 ? "" : "s", type->name, n == 1 ? "has" : "have");
        return;
    }

    if (wrap_only && ti_type_required_by_non_wpo(type, e))
        return;

    if (!wrap_only && ti_type_requires_wpo(type, e))
        return;

    task = ti_task_get_task(query->change, query->collection->root);
    if (!task)
    {
        ex_set_mem(e);
        return;
    }

    ti_type_set_wrap_only_mode(type, wrap_only);

    /* update modified time-stamp */
    type->modified_at = util_now_usec();

    if (ti_task_add_mod_type_wpo(task, type))
        ex_set_mem(e);
}

static void type__hid(
        ti_query_t * query,
        ti_type_t * type,
        cleri_node_t * nd,
        ex_t * e)
{
    static const char * fnname = "mod_type` with task `hid";
    const int nargs = fn_get_nargs(nd);
    _Bool hide_id;
    ti_task_t * task;

    nd = nd->children->next->next->next->next;

    if (fn_nargs(fnname, DOC_MOD_TYPE_HID, 3, nargs, e) ||
        ti_do_statement(query, nd, e) ||
        fn_arg_bool(fnname, DOC_MOD_TYPE_HID, 3, query->rval, e))
        return;

    hide_id = ti_val_as_bool(query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (hide_id == ti_type_hide_id(type))
        return;  /* nothing to do */

    task = ti_task_get_task(query->change, query->collection->root);
    if (!task)
    {
        ex_set_mem(e);
        return;
    }

    ti_type_set_hide_id(type, hide_id);

    /* update modified time-stamp */
    type->modified_at = util_now_usec();

    if (ti_task_add_mod_type_hid(task, type))
        ex_set_mem(e);
}

static int do__f_mod_type(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    ti_type_t * type;
    ti_raw_t * rmod;
    ti_name_t * name = NULL;
    cleri_node_t * child = nd->children;
    const int nargs = fn_get_nargs(nd);

    if (fn_not_collection_scope("mod_type", query, e) ||
        fn_nargs_min("mod_type", DOC_MOD_TYPE, 3, nargs, e) ||
        ti_do_statement(query, child, e) ||
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

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, (child = child->next->next), e) ||
        fn_arg_name_check("mod_type", DOC_MOD_TYPE, 2, query->rval, e))
        goto fail0;

    rmod = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_raw_eq_strn(rmod, "wpo", 3))
    {
        type__wpo(query, type, nd, e);
        goto done;
    }

    if (ti_raw_eq_strn(rmod, "hid", 3))
    {
        type__hid(query, type, nd, e);
        goto done;
    }

    if (ti_raw_eq_strn(rmod, "all", 3))
    {
        type__all(query, type, nd, e);
        goto done;
    }

    /* correct error message #407 (not optimized but fast enough)*/
    if (!ti_raw_eq_strn(rmod, "add", 3) &&
        !ti_raw_eq_strn(rmod, "del", 3) &&
        !ti_raw_eq_strn(rmod, "mod", 3) &&
        !ti_raw_eq_strn(rmod, "ren", 3) &&
        !ti_raw_eq_strn(rmod, "rel", 3))
        goto unknown;

    if (ti_do_statement(query, (child = child->next->next), e) ||
        fn_arg_name_check("mod_type", DOC_MOD_TYPE, 3, query->rval, e))
        goto fail1;

    name = ti_names_from_raw((ti_raw_t *) query->rval);
    if (!name)
    {
        ex_set_mem(e);
        goto fail1;
    }

    ti_val_unsafe_drop(query->rval);
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

    if (ti_raw_eq_strn(rmod, "rel", 3))
    {
        type__rel(query, type, name, nd, e);
        goto done;
    }

unknown:
    ex_set(e, EX_VALUE_ERROR,
            "function `mod_type` expects argument 2 to be "
            "`all`, `add`, `del`, `hid`, `mod`, `rel`, `ren` or `wpo` "
            "but got `%.*s` instead"
            DOC_MOD_TYPE,
            rmod->n, (const char *) rmod->data);

done:
    if (e->nr == 0)
    {
        ti_type_map_cleanup(type);

        ti_val_drop(query->rval);
        query->rval = (ti_val_t *) ti_nil_get();
    }
    ti_name_drop(name);

fail1:
    ti_val_unsafe_drop((ti_val_t *) rmod);
fail0:
    ti_type_unlock(type, true /* lock is set for sure */);
    return e->nr;
}
