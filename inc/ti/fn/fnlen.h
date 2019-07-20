#include <ti/fn/fn.h>

#define LEN_DOC_ TI_SEE_DOC("#len")

static int do__f_len(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * val;

    if (!ti_val_has_len(query->rval))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `len`"LEN_DOC_,
                ti_val_str(query->rval));
        return e->nr;
    }

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `len` takes 0 arguments but %d %s given"LEN_DOC_,
                nargs, nargs == 1 ? "was" : "were");
        return e->nr;
    }

    val = query->rval;
    query->rval = (ti_val_t *) ti_vint_create((int64_t) ti_val_get_len(val));
    if (!query->rval)
        ex_set_mem(e);

    ti_val_drop(val);
    return e->nr;
}
