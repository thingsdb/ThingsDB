#include <ti/fn/fn.h>

static int do__f_return(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_nargs_max("return", DOC_RETURN, 2, nargs, e))
        return e->nr;

    if (nargs == 0)
    {
        ex_set_return(e);
        query->rval = (ti_val_t *) ti_nil_get();
        return e->nr;
    }

    if (nargs == 2)
    {
        int64_t deepi;

        if (ti_do_statement(query, nd->children->next->next->node, e) ||
            fn_arg_int("return", DOC_RETURN, 2, query->rval, e))
            return e->nr;

        deepi = VINT(query->rval);
        if (deepi < 0 || deepi > MAX_DEEP_HINT)
        {
            ex_set(e, EX_VALUE_ERROR,
                    "expecting a `deep` value between 0 and %d "
                    "but got %"PRId64" instead",
                    MAX_DEEP_HINT, deepi);
            return e->nr;
        }

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;

        query->qbind.deep = (uint8_t) deepi;
    }

    if (ti_do_statement(query, nd->children->node, e) == 0)
        ex_set_return(e);  /* on success */

    return e->nr;
}
