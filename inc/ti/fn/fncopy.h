#include <ti/fn/fn.h>

static int do__f_copy(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_thing_t * thing;
    uint8_t deep;

    if (!ti_val_is_thing(query->rval))
        return fn_call_try("copy", query, nd, e);

    if (fn_nargs_max("copy", DOC_THING_ID, 1, nargs, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (nargs == 1)
    {
        int64_t deepi;

        if (ti_do_statement(query, nd->children->next->next->node, e) ||
            fn_arg_int("equals", DOC_THING_EQUALS, 2, query->rval, e))
            goto fail0;

        deepi = VINT(query->rval);
        if (deepi < 0 || deepi > MAX_DEEP_HINT)
        {
            ex_set(e, EX_VALUE_ERROR,
                    "expecting a `deep` value between 0 and %d "
                    "but got %"PRId64" instead",
                    MAX_DEEP_HINT, deepi);
            goto fail0;
        }

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
        deep = (uint8_t) deepi;
    }
    else
        deep = 1;

    if (thing_)


fail0:
    ti_val_unsafe_drop((ti_val_t *) thing);
    return e->nr;
}
