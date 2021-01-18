#include <ti/fn/fn.h>

static int do__f_module_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool with_conf;
    ti_module_t * module;

    if (fn_nargs("module_info", DOC_MODULE_INFO, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("module_info", DOC_MODULE_INFO, 1, query->rval, e))
        return e->nr;

    with_conf = ti_access_check(ti.access_thingsdb, query->user, TI_AUTH_EVENT);

    module = ti_modules_by_raw((ti_raw_t *) query->rval);
    if (!module)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "module", e);

    ti_val_unsafe_drop(query->rval);
    query->rval = ti_module_as_mpval(module, with_conf);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
