#include <ti/fn/fn.h>

#define SET_LOG_LEVEL_DOC_ TI_SEE_DOC("#set_log_level")

static int do__f_set_log_level(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    int log_level;
    int64_t ilog;

    if (fn_not_node_scope("set_log_level", query, e))
        return e->nr;

    assert (!query->ev);    /* node queries do never create an event */

    /* check for privileges */
    if (ti_access_check_err(ti()->access_node,
            query->user, TI_AUTH_MODIFY, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `set_log_level` takes 1 argument but %d were given"
                SET_LOG_LEVEL_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `set_log_level` expects argument 1 to be of "
            "type `"TI_VAL_INT_S"` but got type `%s` instead"SET_LOG_LEVEL_DOC_,
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
