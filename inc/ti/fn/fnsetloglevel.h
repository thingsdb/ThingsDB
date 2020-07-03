#include <ti/fn/fn.h>

static int do__f_set_log_level(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    int log_level;
    int64_t ilog;

    if (fn_not_node_scope("set_log_level", query, e) ||
        ti_access_check_err(ti.access_node,
            query->user, TI_AUTH_MODIFY, e) ||
        fn_nargs("set_log_level", DOC_SET_LOG_LEVEL, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_int("set_log_level", DOC_SET_LOG_LEVEL, 1, query->rval, e))
        return e->nr;

    ilog = VINT(query->rval);

    log_level = ilog < LOGGER_DEBUG
            ? LOGGER_DEBUG
            : ilog > LOGGER_CRITICAL
            ? LOGGER_CRITICAL
            : ilog;

    logger_set_level(log_level);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
