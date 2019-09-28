#include <ti/fn/fn.h>

/* maximum value we allow for the `deep` argument */
#define RETURN_MAX_DEEP_HINT 0x7f

static int do__f_return(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_chained("return", query, e) ||
        fn_nargs_max("return", DOC_RETURN, 2, nargs, e))
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

        if (ti_do_statement(query, nd->children->next->next->node, e))
            return e->nr;

        if (!ti_val_is_int(query->rval))
        {
            ex_set(e, EX_TYPE_ERROR,
                "function `return` expects argument 2 to be of "
                "type `"TI_VAL_INT_S"` but got type `%s` instead"DOC_RETURN,
                ti_val_str(query->rval));
            return e->nr;
        }

        deepi = ((ti_vint_t *) query->rval)->int_;

        if (deepi < 0 || deepi > RETURN_MAX_DEEP_HINT)
        {
            ex_set(e, EX_VALUE_ERROR,
                    "expecting a `deep` value between 0 and %d "
                    "but got %"PRId64" instead",
                    RETURN_MAX_DEEP_HINT, deepi);
            return e->nr;
        }

        ti_val_drop(query->rval);
        query->rval = NULL;

        query->syntax.deep = (uint8_t) deepi;
    }

    if (ti_do_statement(query, nd->children->node, e) == 0)
        ex_set_return(e);  /* on success */

    return e->nr;
}
