#include <ti/cfn/fn.h>

static int cq__f_blob(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    int n_blobs = query->blobs ? query->blobs->n : 0;
    int64_t idx;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `blob` takes 1 argument but %d were given", n);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `blob` expects argument 1 to be of "
                "type `"TI_VAL_INT_S"` but got type `%s` instead",
                ti_val_str(query->rval));
        return e->nr;
    }

    idx = ((ti_vint_t *) query->rval)->int_;

    if (idx < 0)
        idx += n_blobs;

    if (idx < 0 || idx >= n_blobs)
    {
        ex_set(e, EX_INDEX_ERROR, "blob index out of range");
        return e->nr;
    }

    ti_val_drop(query->rval);
    query->rval = vec_get(query->blobs, idx);

    ti_incref(query->rval);
    return e->nr;
}
