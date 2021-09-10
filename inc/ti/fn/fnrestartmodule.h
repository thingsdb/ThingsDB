#include <ti/fn/fn.h>

static int do__f_restart_module(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_module_t * module;

    if (fn_not_node_scope("restart_module", query, e) ||
        ti_access_check_err(
                    ti.access_node,
                    query->user, TI_AUTH_CHANGE, e) ||
        fn_nargs("restart_module", DOC_RESTART_MODULE, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("restart_module", DOC_RESTART_MODULE, 1, query->rval, e))
        return e->nr;

    module = ti_modules_by_raw((ti_raw_t *) query->rval);
    if (!module)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "module", e);

    ti_module_restart(module);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
