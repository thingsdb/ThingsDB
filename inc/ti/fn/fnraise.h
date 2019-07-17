#include <ti/fn/fn.h>

#define RAISE_DOC_ TI_SEE_DOC("#raise")

static int do__f_raise(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        if (nargs == 0)
        {
            assert (query->rval == NULL);
            query->rval = (ti_val_t *) ti_verror_from_code(TI_VERROR_DEF_CODE);
            goto done;
        }
        ex_set(e, EX_BAD_DATA,
                "function `raise` takes at most 1 argument but %d were given"
                RAISE_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_error(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `raise` expects argument 1 to be of "
            "type `"TI_VAL_ERROR_S"` but got type `%s` instead"RAISE_DOC_,
            ti_val_str(query->rval));
        return e->nr;
    }

done:
    ti_verror_to_e((ti_verror_t *) query->rval, e);
    return e->nr;
}
