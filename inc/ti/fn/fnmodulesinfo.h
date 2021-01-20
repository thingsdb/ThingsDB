#include <ti/fn/fn.h>

static int do__f_modules_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    int flags = 0;

    if (fn_nargs("modules_info", DOC_MODULES_INFO, 0, nargs, e))
        return e->nr;

    if (query->qbind.flags & TI_QBIND_FLAG_NODE)
        flags |= TI_MODULE_FLAG_WITH_TASKS;

    if (ti_access_check(ti.access_thingsdb, query->user, TI_AUTH_EVENT))
        flags |= TI_MODULE_FLAG_WITH_CONF;

    query->rval = (ti_val_t *) ti_modules_info(flags);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
