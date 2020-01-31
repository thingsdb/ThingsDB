#include <ti/fn/fn.h>

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
    varr = ti_varr_create(thing->items->n);
    if (!varr)
    {
        ex_set_mem(e);
        return e->nr;
    }

    if (ti_thing_is_object(thing))
    {
        for (vec_each(thing->items, ti_prop_t, prop))
        {
            VEC_push(varr->vec, prop->val);
            ti_incref(prop->val);
        }
    }
    else
    {
        for (vec_each(thing->items, ti_val_t, val))
        {
            VEC_push(varr->vec, val);
            ti_incref(val);
        }
    }

    query->rval = (ti_val_t *) varr;
    ti_val_drop((ti_val_t *) thing);

    return e->nr;
}
