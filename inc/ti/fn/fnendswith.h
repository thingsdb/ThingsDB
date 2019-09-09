#include <ti/fn/fn.h>

#define ENDSWITH_DOC_ TI_SEE_DOC("#endswith")

static int do__f_endswith(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    ti_raw_t * raw;
    _Bool endswith;

    if (fn_not_chained("endswith", query, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `endswith`"ENDSWITH_DOC_,
                ti_val_str(query->rval));
        return e->nr;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `endswith` takes 1 argument but %d were given"
                ENDSWITH_DOC_, nargs);
        return e->nr;
    }

    raw = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e))
        goto failed;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `endswith` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead"ENDSWITH_DOC_,
                ti_val_str(query->rval));
        goto failed;
    }

    endswith = ti_raw_endswith(raw, (ti_raw_t *) query->rval);
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(endswith);

failed:
    ti_val_drop((ti_val_t *) raw);
    return e->nr;
}
