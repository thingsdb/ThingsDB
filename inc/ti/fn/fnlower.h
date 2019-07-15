#include <ti/fn/fn.h>

#define LOWER_DOC_ TI_SEE_DOC("#lower")

static int do__f_lower(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * val = ti_query_val_pop(query);

    if (!ti_val_is_raw(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `lower`"LOWER_DOC_,
                ti_val_str(val));
        goto done;
    }

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `lower` takes 0 arguments but %d %s given"LOWER_DOC_,
                nargs, nargs == 1 ? "was" : "were");
        goto done;
    }

    query->rval = (ti_val_t *) ti_raw_lower((ti_raw_t *) val);
    if (!query->rval)
        ex_set_mem(e);

done:
    ti_val_drop(val);
    return e->nr;
}
