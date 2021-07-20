#include <ti/fn/fn.h>

static inline int assign__set_o(
        ti_thing_t * thing,
        ti_raw_t * key,
        ti_val_t * val,
        ti_task_t * task,
        ex_t * e,
        uint32_t parent_ref)
{
    /*
     * Update the reference count based on the parent. The reason we do this
     * here is that we still require the old value.
     */
    val->ref += parent_ref > 1;

    /*
     * Closures are already unbound so the only possible errors are
     * critical.
     */
    if (ti_val_make_assignable(&val, thing, key, e) ||
        ti_thing_o_set(thing, key, val) ||
        (task && ti_task_add_set(task, key, val)))
        goto failed;

    ti_incref(key);
    val->ref += parent_ref == 1;
    return e->nr;

failed:
    if (parent_ref > 1)
        ti_val_unsafe_gc_drop(val);
    if (e->nr == 0)
        ex_set_mem(e);

    return e->nr;
}

typedef struct
{
    ti_thing_t * thing;
    ti_thing_t * tsrc;
    ti_task_t * task;
    ex_t * e;
} assign__walk_i_t;

static int assign__walk_i(ti_item_t * item, assign__walk_i_t * w)
{
    return assign__set_o(
            w->thing,
            item->key,
            item->val,
            w->task,
            w->e,
            w->tsrc->ref);
}

static int do__f_assign(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    cleri_node_t * name_nd;
    ti_task_t * task = NULL;
    ti_thing_t * thing, * tsrc;
    vec_t * vec = NULL;

    if (!ti_val_is_thing(query->rval))
        return fn_call_try("assign", query, nd, e);

    if (fn_nargs("assign", DOC_THING_ASSIGN, 1, nargs, e))
        return e->nr;

    /*
     * This is a check for `iteration`.
     *
     * // without lock it breaks even with normal variable, luckily map puts
     * // a lock.
     * tmp.map(|| {
     *     tmp.assign({x: 1});
     * }
     */
    if (ti_val_try_lock(query->rval, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    name_nd = nd->children->node;

    if (ti_do_statement(query, name_nd, e) ||
        fn_arg_thing("assign", DOC_THING_ASSIGN, 1, query->rval, e))
    {
        goto fail0;
    }

    tsrc = (ti_thing_t *) query->rval;
    query->rval = (ti_val_t *) thing;
    ti_incref(thing);

    if (thing->id && !(task = ti_task_get_task(query->ev, thing)))
    {
        ex_set_mem(e);
        goto fail1;
    }

    if (ti_thing_is_object(thing))
    {
        if (ti_thing_is_object(tsrc))
        {
            if (ti_thing_is_dict(tsrc))
            {
                assign__walk_i_t w = {
                        .thing = thing,
                        .tsrc = tsrc,
                        .task = task,
                        .e = e,
                };
                if (smap_values(
                        tsrc->items.smap,
                        (smap_val_cb) assign__walk_i,
                        &w))
                    goto fail1;
            }
            else
            {
                for(vec_each(tsrc->items.vec, ti_prop_t, p))
                    if (assign__set_o(
                            thing,
                            (ti_raw_t *) p->name,
                            p->val,
                            task,
                            e,
                            tsrc->ref))
                        goto fail1;
            }
        }
        else
        {
            ti_name_t * name;
            ti_val_t * val;
            for(thing_t_each(tsrc, name, val))
                if (assign__set_o(
                        thing,
                        (ti_raw_t *) name,
                        val,
                        task,
                        e,
                        tsrc->ref))
                    goto fail1;
        }
    }
    else
    {
        ti_type_t * type = thing->via.type;
        uint32_t parent_ref = tsrc->ref;

        if (ti_thing_is_object(tsrc))
        {
            if (ti_thing_is_dict(tsrc))
            {
                ti_raw_t * incompatible;
                if (ti_thing_to_strict(tsrc, &incompatible))
                {
                    if (incompatible)
                        ex_set(e, EX_LOOKUP_ERROR,
                                "type `%s` has no property `%s`",
                                type->name,
                                ti_raw_as_printable_str(incompatible));
                    else
                        ex_set_mem(e);

                    goto fail1;
                }
            }
            vec = vec_new(ti_thing_n(tsrc));
            if (!vec)
            {
                ex_set_mem(e);
                goto fail1;
            }

            for(vec_each(tsrc->items.vec, ti_prop_t, p))
            {
                ti_val_t * val = p->val;
                ti_field_t * field = ti_field_by_name(type, p->name);
                if (!field)
                {
                    ex_set(e, EX_LOOKUP_ERROR,
                            "type `%s` has no property `%s`",
                            type->name, p->name->str);
                    goto fail2;
                }

                if (ti_field_has_relation(field))
                {
                    ex_set(e, EX_TYPE_ERROR,
                            "function `assign` is not able to set "
                            "`%s` because a relation for this "
                            "property is configured"DOC_THING_ASSIGN,
                            p->name->str);
                    goto fail2;
                }

                /*
                 * Update the reference count based on the parent.
                 * The reason we do this here is that we still require the
                 * old value.
                 */
                val->ref += parent_ref > 1;

                if (ti_field_make_assignable(field, &val, thing, e))
                {
                    if (parent_ref > 1)
                        ti_val_unsafe_gc_drop(val);
                    goto fail2;
                }

                val->ref += parent_ref == 1;

                VEC_push(vec, val);
            }

            for(vec_each_rev(tsrc->items.vec, ti_prop_t, p))
            {
                ti_field_t * field = ti_field_by_name(type, p->name);
                ti_val_t * val = VEC_pop(vec);
                ti_thing_t_prop_set(thing, field, val);
                if (task && ti_task_add_set(
                        task,
                        (ti_raw_t *) field->name,
                        val))
                    goto fail2;
            }
        }
        else
        {
            ti_type_t * f_type = tsrc->via.type;

            vec = NULL;

            if (f_type != type)
            {
                ex_set(e, EX_TYPE_ERROR,
                        "cannot assign properties to instance of type `%s` "
                        "from type `%s`"
                        DOC_THING_ASSIGN,
                        type->name,
                        f_type->name);
                goto fail1;
            }

            for (vec_each(type->fields, ti_field_t, field))
            {
                ti_val_t * val = VEC_get(tsrc->items.vec, field->idx);

                val->ref += parent_ref > 1;

                if (ti_field_make_assignable(field, &val, thing, e))
                {
                    if (parent_ref > 1)
                        ti_val_unsafe_gc_drop(val);
                    goto fail1;
                }

                val->ref += parent_ref == 1;

                ti_thing_t_prop_set(thing, field, val);
                if (task && ti_task_add_set(
                        task,
                        (ti_raw_t *) field->name,
                        val))
                    goto fail1;
            }
        }
    }

fail2:
    vec_destroy(vec, (vec_destroy_cb) ti_val_unsafe_drop);
fail1:
    ti_val_unsafe_drop((ti_val_t *) tsrc);
fail0:
    ti_val_unlock((ti_val_t *) thing, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) thing);
    return e->nr;
}
