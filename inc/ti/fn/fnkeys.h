#include <ti/fn/fn.h>

static int keys__walk(ti_item_t * item, vec_t * vec)
{
    VEC_push(vec, item->key);
    ti_incref(item->key);
    return 0;
}

static int keys__items(ti_val_t * k, ti_val_t * UNUSED(v), vec_t * vec)
{
    VEC_push(vec, k);
    ti_incref(k);
    return 0;
}

static int do__f_keys_thing(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_thing_t * thing;
    ti_varr_t * varr;

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

static int do__f_keys_dict(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_dict_t * dict;
    ti_varr_t * varr;

    if (fn_nargs("keys", DOC_DICT_KEYS, 0, nargs, e))
        return e->nr;

    dict = (ti_dict_t *) query->rval;
    varr = ti_varr_create(ti_dict_n(dict));
    if (!varr)
    {
        ex_set_mem(e);
        return e->nr;
    }

    if (ti_dict_items(dict, (ti_dict_item_cb) keys__items, varr->vec))
        goto mem_error;

    query->rval = (ti_val_t *) varr;
    ti_val_unsafe_drop((ti_val_t *) dict);

    return e->nr;

mem_error:
    ex_set_mem(e);
    ti_val_unsafe_drop((ti_val_t *) varr);
    return e->nr;

}

static int do__f_keys(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    return ti_val_is_thing(query->rval)
            ? do__f_keys_thing(query, nd, e)
            : ti_val_is_dict(query->rval)
            ? do__f_keys_dict(query, nd, e)
            : fn_call_try("keys", query, nd, e);
}
