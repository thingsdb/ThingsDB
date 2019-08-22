#include <ti/fn/fn.h>

#define RETURN_DOC_ TI_SEE_DOC("#return")

/* maximum value we allow for the `deep` argument */
#define RETURN_MAX_DEEP_HINT 0x7f

static int do__f_return(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    const int nargs = langdef_nd_n_function_params(nd);

    if (nargs > 2)
    {
        ex_set(e, EX_BAD_DATA,
                "function `return` takes at most 2 arguments but %d were given"
                RETURN_DOC_, nargs);
        return e->nr;
    }

    assert (query->rval == NULL);

    if (nargs == 0)
    {
        ex_set_return(e);
        query->rval = (ti_val_t *) ti_nil_get();
        return e->nr;
    }

    if (nargs == 2)
    {
        int64_t deepi;

        if (ti_do_scope(query, nd->children->next->next->node, e))
            return e->nr;

        if (!ti_val_is_int(query->rval))
        {
            ex_set(e, EX_BAD_DATA,
                "function `return` expects argument 2 to be of "
                "type `"TI_VAL_INT_S"` but got type `%s` instead"RETURN_DOC_,
                ti_val_str(query->rval));
            return e->nr;
        }

        deepi = ((ti_vint_t *) query->rval)->int_;

        if (deepi < 0 || deepi > RETURN_MAX_DEEP_HINT)
        {
            ex_set(e, EX_BAD_DATA,
                    "expecting a `deep` value between 0 and %d "
                    "but got %"PRId64" instead",
                    RETURN_MAX_DEEP_HINT, deepi);
            return e->nr;
        }

        ti_val_drop(query->rval);
        query->rval = NULL;

        query->syntax.deep = (uint8_t) deepi;
    }

    if (ti_do_scope(query, nd->children->node, e) == 0)
        ex_set_return(e);  /* on success */

    return e->nr;
}
