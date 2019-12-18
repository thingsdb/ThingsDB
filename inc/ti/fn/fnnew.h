#include <ti/fn/fn.h>

static int fn_new_instance(
        ti_query_t * query,
        cleri_node_t * nd,
        ti_type_t * type,
        ex_t * e)
{
    ti_thing_t * new_thing, * from_thing;
    int lock_was_set = ti_type_ensure_lock(type);

    (void) ti_do_statement(query, nd, e);
    /* make sure we unlock */
    ti_type_unlock(type, lock_was_set);

    if (e->nr)
        return e->nr;

    if (!ti_val_is_thing(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "new instances can be created from type `"TI_VAL_THING_S"` but "
            "got type `%s` instead"DOC_NEW,
            ti_val_str(query->rval));
        return e->nr;
    }

    from_thing = (ti_thing_t *) query->rval;

    new_thing = ti_thing_t_create(0, type, query->collection);
    if (!new_thing)
    {
        ex_set_mem(e);
        return e->nr;
    }

    if (ti_thing_is_object(from_thing))
    {
        ti_val_t * val;
        for (vec_each(type->fields, ti_field_t, field))
        {
            val = ti_thing_o_val_weak_get(from_thing, field->name);
            if (!val)
            {
                if (field->spec & TI_SPEC_NILLABLE)
                {
                    val = (ti_val_t *) ti_nil_get();
                }
                else
                {
                    ex_set(e, EX_LOOKUP_ERROR,
                            "cannot create type `%s`; "
                            "property `%s` is missing",
                            type->name,
                            field->name->str);
                    goto failed;
                }
            }
            else
            {
                if (ti_field_make_assignable(field, &val, e))
                    goto failed;

                ti_incref(val);
            }
            VEC_push(new_thing->items, val);
        }
    }
    else
    {
        ti_type_t * f_type = ti_thing_type(from_thing);
        if (f_type != type)
        {
            ex_set(e, EX_TYPE_ERROR,
                    "cannot create type `%s` from type `%s`"DOC_NEW_TYPE,
                    type->name,
                    f_type->name);
            goto failed;
        }

        for (vec_each(from_thing->items, ti_val_t, val))
        {
            if (ti_val_make_assignable(&val, e))
                goto failed;

            ti_incref(val);
            VEC_push(new_thing->items, val);
        }
    }

    ti_val_drop(query->rval);  /* from_thing */
    query->rval = (ti_val_t *) new_thing;

    return e->nr;

failed:
    assert (e->nr);
    ti_val_drop((ti_val_t *) new_thing);
    return e->nr;
}

static int do__f_new(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_type_t * type;

    if (fn_not_collection_scope("new", query, e) ||
        fn_nargs("new", DOC_NEW, 2, nargs, e))
        return e->nr;

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `new_type` expects argument 1 to be of "
            "type `"TI_VAL_STR_S"` but got type `%s` instead"DOC_NEW_TYPE,
            ti_val_str(query->rval));
        return e->nr;
    }

    type = ti_types_by_raw(query->collection->types, (ti_raw_t *) query->rval);
    if (!type)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "type", e);

    ti_val_drop(query->rval);
    query->rval = NULL;

    return fn_new_instance(query, nd->children->next->next->node, type, e);
}
