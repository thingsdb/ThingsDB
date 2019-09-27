#include <ti/fn/fn.h>

static int do__f_id(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_thing_t * thing;

    if (fn_not_chained("id", query, e))
        return e->nr;

    if (!ti_val_is_thing(query->rval))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no function `id`"DOC_ID,
                ti_val_str(query->rval));
        return e->nr;
    }

    if (fn_nargs("id", DOC_ID, 0, nargs, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = thing->id
            ? (ti_val_t *) ti_vint_create((int64_t) thing->id)
            : (ti_val_t *) ti_nil_get();

    if (!query->rval)
        ex_set_mem(e);

    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}
