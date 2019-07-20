#include <ti/fn/fn.h>

#define INDEXOF_DOC_ TI_SEE_DOC("#indexof")

static int do__f_indexof(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    size_t idx = 0;
    ti_varr_t * varr;

    if (!ti_val_is_list(query->rval))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `indexof`"INDEXOF_DOC_,
                ti_val_str(query->rval));
        return e->nr;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `indexof` takes 1 argument but %d were given"
                INDEXOF_DOC_, nargs);
        return e->nr;
    }

    varr = (ti_varr_t *) query->rval;
    query->rval = NULL;

    if (ti_do_scope(query, nd->children->node, e))
        goto fail1;

    for (vec_each(varr->vec, ti_val_t, v), ++idx)
    {
        if (ti_opr_eq(v, query->rval))
        {
            ti_val_drop(query->rval);
            query->rval = (ti_val_t *) ti_vint_create(idx);
            if (!query->rval)
                ex_set_mem(e);
            goto done;
        }
    }

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

done:
fail1:
    ti_val_drop((ti_val_t *) varr);
    return e->nr;
}
