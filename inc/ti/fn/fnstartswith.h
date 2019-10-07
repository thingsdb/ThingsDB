#include <ti/fn/fn.h>

static int do__f_startswith(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * raw;
    _Bool startswith;

    if (fn_not_chained("startswith", query, e))
        return e->nr;

    if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no function `startswith`"DOC_STARTSWITH,
                ti_val_str(query->rval));
        return e->nr;
    }

    if (fn_nargs("startswith", DOC_STARTSWITH, 1, nargs, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e))
        goto failed;

    if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `startswith` expects argument 1 to be of "
                "type `"TI_VAL_STR_S"` but got type `%s` instead"
                DOC_STARTSWITH, ti_val_str(query->rval));
        goto failed;
    }

    startswith = ti_raw_startswith(raw, (ti_raw_t *) query->rval);
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(startswith);

failed:
    ti_val_drop((ti_val_t *) raw);
    return e->nr;
}
