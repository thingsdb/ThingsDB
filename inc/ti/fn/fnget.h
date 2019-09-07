#include <ti/fn/fn.h>

#define GET_DOC_ TI_SEE_DOC("#get")

static int do__f_get(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    ti_thing_t * thing;
    ti_prop_t * prop;

    if (fn_not_chained("get", query, e))
        return e->nr;

    if (!ti_val_is_thing(query->rval))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `get`"GET_DOC_,
                ti_val_str(query->rval));
        return e->nr;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `get` takes 1 argument but %d were given"
                GET_DOC_, nargs);
        return e->nr;
    }

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_scope(query, nd->children->node, e))
        goto fail0;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `get` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"GET_DOC_,
            ti_val_str(query->rval));
        goto fail0;
    }

    prop = ti_thing_weak_get_e(thing, (ti_raw_t *) query->rval, e);
    if (!prop)
        goto fail0;

    if (thing->id)
        ti_chain_set(&query->chain, thing, prop->name);

    ti_val_drop(query->rval);
    query->rval = prop->val;
    ti_incref(query->rval);

fail0:
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}
