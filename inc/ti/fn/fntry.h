#include <ti/fn/fn.h>

static int do__f_try(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    cleri_children_t * child = nd->children;    /* first in argument list */
    const int nargs = langdef_nd_n_function_params(nd);
    ex_enum errnr;
    ti_verror_t * verror;

    if (fn_chained("try", query, e) ||
        fn_nargs_min("try", DOC_TRY, 1, nargs, e))
        return e->nr;

    errnr = ti_do_statement(query, child->node, e);

    if (errnr > EX_MAX_BUILD_IN_ERR && errnr <= EX_RETURN)
        return errnr;   /* do not catch success or internal errors */

    verror = (ti_verror_t *) ti_verror_from_e(e);
    if (!verror)
    {
        ex_set_mem(e);
        return e->nr;
    }

    /* make sure the return value is dropped */
    ti_val_drop(query->rval);

    e->nr = 0;

    if (nargs == 1)
    {
        query->rval = (ti_val_t *) verror;
        assert (e->nr == 0);
        return e->nr;
    }

    query->rval = NULL;
    child = child->next->next;

    while (1)
    {
        if (ti_do_statement(query, child->node, e))
            goto failed;

        if (!ti_val_is_error(query->rval))
        {
            ex_set(e, EX_TYPE_ERROR,
                "function `try` expects arguments 2..X to be of "
                "type `"TI_VAL_ERROR_S"` but got type `%s` instead"DOC_TRY,
                ti_val_str(query->rval));
            goto failed;
        }

        if ((ex_enum) ((ti_verror_t * ) query->rval)->code == errnr)
        {
            ti_val_drop(query->rval);
            query->rval = (ti_val_t *) verror;
            assert (e->nr == 0);
            return e->nr;
        }

        ti_val_drop(query->rval);
        query->rval = NULL;

        if (!child->next || !(child = child->next->next))
            break;
    }

    ti_verror_to_e(verror, e);

failed:
    ti_val_drop((ti_val_t *) verror);
    return e->nr;
}
