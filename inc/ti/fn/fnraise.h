#include <ti/fn/fn.h>

static int do__f_raise(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_chained("raise", query, e) ||
        fn_nargs_max("raise", DOC_RAISE, 1, nargs, e))
        return e->nr;

    if (nargs == 0)
    {
        assert (query->rval == NULL);
        query->rval = (ti_val_t *) ti_verror_from_code(TI_VERROR_DEF_CODE);
        goto done;
    }

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_error(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `raise` expects argument 1 to be of "
            "type `"TI_VAL_ERROR_S"` but got type `%s` instead"DOC_RAISE,
            ti_val_str(query->rval));
        return e->nr;
    }

done:
    ti_verror_to_e((ti_verror_t *) query->rval, e);
    return e->nr;
}
