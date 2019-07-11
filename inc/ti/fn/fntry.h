#include <ti/fn/fn.h>

#define TRY_DOC_ TI_SEE_DOC("#try")

static int do__f_try(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    cleri_children_t * child = nd->children;    /* first in argument list */
    const int nargs = langdef_nd_n_function_params(nd);
    ex_enum errnr;
    cleri_node_t * alt_node;

    if (nargs < 1)
    {
        ex_set(e, EX_BAD_DATA,
                "function `try` requires at least 1 argument but 0 "
                "were given"TRY_DOC_);
        return e->nr;
    }

    errnr = ti_do_scope(query, child->node, e);

    if (errnr > -100)
        return errnr;   /* return when successful or internal errors */

    ti_val_drop(query->rval);
    e->nr = 0;

    if (nargs == 1)
    {
        query->rval = (ti_val_t *) ti_nil_get();
        return 0;
    }

    query->rval = NULL;
    child = child->next->next;
    alt_node = child->node;

    if (nargs == 2)
        goto returnalt;

    for (child = child->next->next; child; child = child->next->next)
    {
        if (ti_do_scope(query, child->node, e))
            return e->nr;

        if (ti_val_convert_to_errnr(&query->rval, e))
            return e->nr;

        assert (query->rval->tp == TI_VAL_INT);

        if ((ex_enum) ((ti_vint_t * ) query->rval)->int_ == errnr)
        {
            ti_val_drop(query->rval);
            query->rval = NULL;
            goto returnalt;  /* catch the error */
        }

        if (!child->next)
            break;
    }

    e->nr = errnr;
    return e->nr;

returnalt:
    /* lazy evaluation of the alternative return value */
    (void) ti_do_scope(query, alt_node, e);
    return e->nr;
}
