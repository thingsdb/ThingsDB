#include <ti/fn/fn.h>

static int do__f_modules_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool with_conf;
    ti_module_t * module;

    if (fn_nargs("modules_info", DOC_MODULES_INFO, 0, nargs, e))
        return e->nr;

    with_conf = ti_access_check(ti.access_thingsdb, query->user, TI_AUTH_EVENT);

    query->rval = ti_modules_info(with_conf);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
