#include <ti/fn/fn.h>

static int values__walk_i(ti_item_t * item, vec_t * vec)
{
    VEC_push(vec, item->val);
    ti_incref(item->val);
    return 0;
}

static int do__f_values(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_thing_t * thing;
    ti_varr_t * varr;

    if (!ti_val_is_thing(query->rval))
        return fn_call_try("values", query, nd, e);

    if (fn_nargs("values", DOC_THING_VALUES, 0, nargs, e))
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
                    (smap_val_cb) values__walk_i,
                    varr->vec);
        }
        else
        {
            for (vec_each(thing->items.vec, ti_prop_t, prop))
            {
                VEC_push(varr->vec, prop->val);
                ti_incref(prop->val);
            }
        }
    }
    else
    {
        for (vec_each(thing->items.vec, ti_val_t, val))
        {
            VEC_push(varr->vec, val);
            ti_incref(val);
        }
    }

    query->rval = (ti_val_t *) varr;
    ti_val_unsafe_drop((ti_val_t *) thing);

    return e->nr;
}
