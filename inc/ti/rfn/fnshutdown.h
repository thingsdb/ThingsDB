#include <ti/rfn/fn.h>

static int rq__f_shutdown(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_node(query, nd, e));
    assert (!query->ev);
    assert (e->nr == 0);
    assert (query->rval == NULL);

    /* check for privileges */
    if (ti_access_check_err(ti()->access_node,
            query->user, TI_AUTH_MODIFY, e))
        return e->nr;

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `shutdown` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    ti_term(SIGINT);

    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}