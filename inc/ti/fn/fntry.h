#include <ti/fn/fn.h>

#define TRY_DOC_ TI_SEE_DOC("#try")

static int do__f_try(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    cleri_children_t * child = nd->children;    /* first in argument list */
    const int nargs = langdef_nd_n_function_params(nd);
    ex_enum errnr;
    ti_verror_t * verror;
    _Bool catch;

    if (nargs < 1)
    {
        ex_set(e, EX_BAD_DATA,
                "function `try` requires at least 1 argument but 0 "
                "were given"TRY_DOC_);
        return e->nr;
    }

    errnr = ti_do_scope(query, child->node, e);

    if (errnr >= -99 && errnr <= 0)
        return errnr;   /* return when successful or internal errors */

    /* make sure the return value is dropped */
    ti_val_drop(query->rval);

    verror = (ti_verror_t *) ti_verror_from_e(e);
    if (!verror)
    {
        ex_set_mem(e);
        return e->nr;
    }

    e->nr = 0;

    if (nargs == 1)
    {
        LOGC("HERE...");
        query->rval = (ti_val_t *) verror;
        assert (e->nr == 0);
        return e->nr;
    }

    query->rval = NULL;
    child = child->next->next;

    for (child = child->next->next; child; child = child->next->next)
    {
        if (ti_do_scope(query, child->node, e))
            goto failed;

        if (!ti_val_is_error(query->rval))
        {
            ex_set(e, EX_BAD_DATA,
                "function `error` expects arguments 2 to X to be of "
                "type `"TI_VAL_ERROR_S"` but got type `%s` instead"
                TRY_DOC_,
                ti_val_str(query->rval));
            goto failed;
        }

        catch = (ex_enum) ((ti_verror_t * ) query->rval)->code == errnr;
        ti_val_drop(query->rval);

        if (catch)
        {
            query->rval = (ti_val_t *) verror;
            assert (e->nr == 0);
            return e->nr;
        }

        query->rval = NULL;

        if (!child->next)
            break;
    }

    ti_verror_to_e(verror, e);

failed:
    ti_val_drop((ti_val_t *) verror);
    return e->nr;
}
