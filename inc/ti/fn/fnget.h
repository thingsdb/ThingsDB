#include <ti/fn/fn.h>

#define GET_DOC_ TI_SEE_DOC("#get")

static int do__f_get(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
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

    if (nargs < 1)
    {
        ex_set(e, EX_BAD_DATA,
                "function `get` requires at least 1 argument but 0 "
                "were given"GET_DOC_);
        return e->nr;
    }

    if (nargs > 2)
    {
        ex_set(e, EX_BAD_DATA,
                "function `get` takes at most 2 arguments but %d "
                "were given"GET_DOC_, nargs);
        return e->nr;
    }

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e))
        goto done;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `get` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"GET_DOC_,
            ti_val_str(query->rval));
        goto done;
    }

    prop = ti_thing_weak_get(thing, (ti_raw_t *) query->rval);
    ti_val_drop(query->rval);

    if (!prop)
    {
        if (nargs == 2)
        {
            query->rval = NULL;
            (void) ti_do_statement(query, nd->children->next->next->node, e);
            goto done;
        }

        query->rval = (ti_val_t *) ti_nil_get();
        goto done;
    }

    if (thing->id)
        ti_chain_set(&query->chain, thing, prop->name);

    query->rval = prop->val;
    ti_incref(query->rval);

done:
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}
