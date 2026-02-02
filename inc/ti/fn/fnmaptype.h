#include <ti/fn/fn.h>


static int do__f_map_type(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_varr_t * varr;
    ti_type_t * type;
    ti_varr_t * vsource;

    if (!ti_val_is_array(query->rval))
        return fn_call_try("map_type", query, nd, e);

    if (fn_nargs("map_type", DOC_LIST_MAP_TYPE, 1, nargs, e))
        return e->nr;

    vsource = (ti_varr_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e) ||
        fn_arg_str("map_type", DOC_LIST_MAP_TYPE, 1, query->rval, e))
        goto fail0;

    type = query->collection
        ? ti_types_by_raw(query->collection->types, (ti_raw_t *) query->rval)
        : NULL;
    if (!type)
    {
        (void) ti_raw_err_not_found((ti_raw_t *) query->rval, "type", e);
        goto fail0;
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    varr = ti_varr_create(vsource->vec->n);
    if (!varr)
        goto fail0;

    for (vec_each(vsource->vec, ti_val_t, val))
    {
        ti_thing_t * thing;

        if (ti_val_is_int(val))
        {
            uint64_t id = VINT(val);
            thing = ti_query_thing_from_id(query, id, e);
            if (!thing)
                goto fail0;

            if (thing->type_id != type->type_id)
            {
                ex_set(e, EX_TYPE_ERROR,
                        TI_THING_ID" is of type `%s`, not `%s`",
                        thing->id, ti_val_str((ti_val_t *) thing), type->name);
                ti_decref(thing);
                goto fail1;
            }
        }
        else if (ti_val_is_thing(val))
        {
            thing = ti_type_from_thing(type, (ti_thing_t *) val, e);
            if (!thing)
                goto fail1;
        }
        else
        {
            ex_set(e, EX_TYPE_ERROR,
                    "function `map_type` cannot convert type `%s` to `%s`",
                    ti_val_str(val),
                    type->name);
            goto fail1;
        }

        VEC_push(varr->vec, thing);
    }

    /* fixed bug #434, must set MHT */
    varr->flags |= TI_VARR_FLAG_MHT;

    ti_val_unsafe_drop((ti_val_t *) vsource);
    query->rval = (ti_val_t *) varr;
    return e->nr;

fail1:
    ti_val_unsafe_drop((ti_val_t *) varr);
fail0:
    ti_val_unsafe_drop((ti_val_t *) vsource);
    return e->nr;
}
