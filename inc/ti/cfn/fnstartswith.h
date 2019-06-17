#include <ti/cfn/fn.h>

static int cq__f_startswith(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * val = ti_query_val_pop(query);
    _Bool startswith;

    if (!ti_val_is_raw(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `startswith`",
                ti_val_str(val));
        goto done;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `startswith` takes 1 argument but %d were given", n);
        goto done;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        goto done;

    if (query->rval->tp != TI_VAL_RAW)
    {
        ex_set(e, EX_BAD_DATA,
                "function `startswith` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead",
                ti_val_str(query->rval));
        goto done;
    }

    startswith = ti_raw_startswith((ti_raw_t *) val, (ti_raw_t *) query->rval);
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(startswith);

done:
    ti_val_drop(val);
    return e->nr;
}
