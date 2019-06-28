#include <ti/cfn/fn.h>

#define CONTAINS_DOC_ TI_SEE_DOC("#contains")

static int cq__f_contains(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * val = ti_query_val_pop(query);
    _Bool contains;

    if (!ti_val_is_raw(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `contains`"CONTAINS_DOC_,
                ti_val_str(val));
        goto done;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `contains` takes 1 argument but %d were given"
                CONTAINS_DOC_, nargs);
        goto done;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        goto done;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `contains` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead"CONTAINS_DOC_,
                ti_val_str(query->rval));
        goto done;
    }

    contains = ti_raw_contains((ti_raw_t *) val, (ti_raw_t *) query->rval);
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(contains);

done:
    ti_val_drop(val);
    return e->nr;
}
