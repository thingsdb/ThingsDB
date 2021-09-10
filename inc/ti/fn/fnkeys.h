#include <ti/fn/fn.h>

static int keys__walk(ti_item_t * item, vec_t * vec)
{
    VEC_push(vec, item->key);
    ti_incref(item->key);
    return 0;
}

static int do__f_keys(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_thing_t * thing;
    ti_varr_t * varr;

    if (!ti_val_is_thing(query->rval))
        return fn_call_try("keys", query, nd, e);

    if (fn_nargs("keys", DOC_THING_KEYS, 0, nargs, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    varr = ti_varr_create(ti_thing_n(thing));
    if (!varr)
    {
        ex_set_mem(e);
        return e->nr;
    }

    if (ti_thing_is_object(thing))
    {
        if (ti_thing_is_dict(thing))
        {
            (void) smap_values(
                    thing->items.smap,
                    (smap_val_cb) keys__walk,
                    varr->vec);
        }
        else
        {
            for (vec_each(thing->items.vec, ti_prop_t, prop))
            {
                VEC_push(varr->vec, prop->name);
                ti_incref(prop->name);
            }
        }
    }
    else
    {
        for (vec_each(thing->via.type->fields, ti_field_t, field))
        {
            VEC_push(varr->vec, field->name);
            ti_incref(field->name);
        }
    }
    query->rval = (ti_val_t *) varr;
    ti_val_unsafe_drop((ti_val_t *) thing);

    return e->nr;
}
