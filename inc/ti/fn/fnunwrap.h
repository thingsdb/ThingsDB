#include <ti/fn/fn.h>

static int do__f_unwrap(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_thing_t * thing;

    if (fn_not_chained("unwrap", query, e))
        return e->nr;

    if (!ti_val_is_wrap(query->rval))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no function `unwrap`"DOC_WTYPE_UNWRAP,
                ti_val_str(query->rval));
        return e->nr;
    }

    if (fn_nargs("unwrap", DOC_WTYPE_UNWRAP, 0, nargs, e))
        return e->nr;

    thing = ((ti_wrap_t *) query->rval)->thing;
    ti_incref(thing);
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) thing;

    return 0;
}
