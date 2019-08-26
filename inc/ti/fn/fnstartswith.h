#include <ti/fn/fn.h>

#define STARTSWITH_DOC_ TI_SEE_DOC("#startswith")

static int do__f_startswith(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    ti_raw_t * raw;
    _Bool startswith;

    if (fn_not_chained("startswith", query, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `startswith`"STARTSWITH_DOC_,
                ti_val_str(query->rval));
        return e->nr;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `startswith` takes 1 argument but %d were given"
                STARTSWITH_DOC_, nargs);
        return e->nr;
    }

    raw = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_scope(query, nd->children->node, e))
        goto failed;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `startswith` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead"
                STARTSWITH_DOC_, ti_val_str(query->rval));
        goto failed;
    }

    startswith = ti_raw_startswith(raw, (ti_raw_t *) query->rval);
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(startswith);

failed:
    ti_val_drop((ti_val_t *) raw);
    return e->nr;
}
