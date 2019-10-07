#include <ti/fn/fn.h>

static int do__f_contains(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * raw;
    _Bool contains;

    if (fn_not_chained("contains", query, e))
        return e->nr;

    if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no function `contains`"DOC_CONTAINS,
                ti_val_str(query->rval));
        return e->nr;
    }

    if (fn_nargs("contains", DOC_CONTAINS, 1, nargs, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e))
        goto failed;

    if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `contains` expects argument 1 to be of "
                "type `"TI_VAL_STR_S"` but got type `%s` instead"DOC_CONTAINS,
                ti_val_str(query->rval));
        goto failed;
    }

    contains = ti_raw_contains(raw, (ti_raw_t *) query->rval);
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(contains);

failed:
    ti_val_drop((ti_val_t *) raw);
    return e->nr;
}
