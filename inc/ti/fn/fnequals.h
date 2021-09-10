#include <ti/fn/fn.h>

static int do__f_equals(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    _Bool equals;
    ti_thing_t * thing;
    ti_val_t * other;
    uint8_t deep;

    if (!ti_val_is_thing(query->rval))
        return fn_call_try("equals", query, nd, e);

    if (fn_nargs_range("equals", DOC_THING_EQUALS, 1, 2, nargs, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e))
        goto fail0;

    other = query->rval;
    query->rval = NULL;

    if (nargs == 2)
    {
        int64_t deepi;

        if (ti_do_statement(query, nd->children->next->next->node, e) ||
            fn_arg_int("equals", DOC_THING_EQUALS, 2, query->rval, e))
            goto fail1;

        deepi = VINT(query->rval);
        if (deepi < 0 || deepi > TI_MAX_DEEP_HINT)
        {
            ex_set(e, EX_VALUE_ERROR,
                    "expecting a `deep` value between 0 and %d "
                    "but got %"PRId64" instead",
                    TI_MAX_DEEP_HINT, deepi);
            goto fail1;
        }

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
        deep = (uint8_t) deepi;
    }
    else
        deep = 1;

    equals = ti_thing_equals(thing, other, deep);
    query->rval = (ti_val_t *) ti_vbool_get(equals);

fail1:
    ti_val_unsafe_drop(other);
fail0:
    ti_val_unsafe_drop((ti_val_t *) thing);
    return e->nr;
}
