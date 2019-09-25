#include <ti/fn/fn.h>

static int do__f_new(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_type_t * type;
    ti_thing_t * new_thing, * from_thing;

    if (fn_not_collection_scope("new", query, e) ||
        fn_nargs("new", DOC_NEW, 2, nargs, e))
        return e->nr;

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `new_type` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"DOC_NEW_TYPE,
            ti_val_str(query->rval));
        return e->nr;
    }

    type = ti_types_by_raw(query->collection->types, (ti_raw_t *) query->rval);
    if (!type)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "type", e);

    ti_val_drop((ti_raw_t *) query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next->node, e))
        return e->nr;

    if (!ti_val_is_thing(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `new_type` expects argument 1 to be of "
            "type `"TI_VAL_THING_S"` but got type `%s` instead"DOC_NEW,
            ti_val_str(query->rval));
        return e->nr;
    }

    from_thing = (ti_thing_t *) query->rval;

    new_thing = ti_thing_t_create(0, type, query->collection);

    if (ti_thing_is_object(from_thing))
    {
        for (vec_each(type->fields, ti_field_t, field))
        {
            ti_val_t * val = ti_thing_o_weak_val_by_name(from_thing, field->name);
            if (ti_field_check(field, val, e))
                goto failed;

            ti_incref(val);
            VEC_push(new_thing->items, val);
        }
    }
    else
    {
        vec_t * cast;
        for (vec_each(cast, uintptr_t, idx))
        {
            VEC_push(new_thing->items, vec_get(from_thing->items, *idx));
        }
    }

    ti_val_drop((ti_raw_t *) query->rval);
    query->rval = (ti_val_t *) thing;

    return e->nr;

failed:
    assert (e->nr);
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}
