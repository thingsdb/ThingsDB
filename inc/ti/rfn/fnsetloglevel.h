#include <ti/rfn/fn.h>

static int rq__f_set_loglevel(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_node(query, nd, e));
    assert (!query->ev);
    assert (e->nr == 0);
    assert (query->rval == NULL);
    int log_level;
    int64_t ilog;

    /* check for privileges */
    if (ti_access_check_err(ti()->access_node,
            query->user, TI_AUTH_MODIFY, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `set_loglevel` takes 1 argument but %d were given",
                n);
        return e->nr;
    }

    if (rq__scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `set_loglevel` expects argument 1 to be of "
            "type `"TI_VAL_INT_S"` but got type `%s` instead",
            ti_val_str(query->rval));
        return e->nr;
    }

    ilog = ((ti_vint_t *) query->rval)->int_;

    log_level = ilog < LOGGER_DEBUG
            ? LOGGER_DEBUG
            : ilog > LOGGER_CRITICAL
            ? LOGGER_CRITICAL
            : ilog;

    logger_set_level(log_level);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
