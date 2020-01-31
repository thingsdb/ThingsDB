#include <ti/fn/fn.h>

static int do__f_get(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_thing_t * thing;
    ti_wprop_t wprop;
    _Bool found;

    if (!ti_val_is_thing(query->rval))
        return fn_call_try("get", query, nd, e);

    if (fn_nargs_range("get", DOC_THING_GET, 1, 2, nargs, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e))
        goto done;

    if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `get` expects argument 1 to be of "
            "type `"TI_VAL_STR_S"` but got type `%s` instead"DOC_THING_GET,
            ti_val_str(query->rval));
        goto done;
    }

    found = ti_thing_get_by_raw(&wprop, thing, (ti_raw_t *) query->rval);

    ti_val_drop(query->rval);

    if (!found)
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
        ti_chain_set(&query->chain, thing, wprop.name);

    query->rval = *wprop.val;
    ti_incref(query->rval);

done:
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}
