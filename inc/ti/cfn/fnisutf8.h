#include <ti/cfn/fn.h>

static int cq__f_isutf8(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool is_utf8;
    ti_raw_t * raw;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `isutf8` takes 1 argument but %d were given", n);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    is_utf8 = ti_val_is_raw(query->rval) &&
            strx_is_utf8n((const char *) raw->data, raw->n);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_utf8);

    return e->nr;
}
