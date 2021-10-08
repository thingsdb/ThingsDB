#include <ti/fn/fn.h>

static int do__f_refresh_module(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_module_t * module;
    ti_task_t * task;
    ti_raw_t * rname;

    if (fn_not_thingsdb_scope("refresh_module", query, e) ||
        fn_nargs("refresh_module", DOC_REFRESH_MODULE, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str_slow("refresh_module", DOC_REFRESH_MODULE, 1, query->rval, e))
        return e->nr;

    rname = (ti_raw_t *) query->rval;
    module = ti_modules_by_raw(rname);
    if (!module)
        return ti_raw_err_not_found(rname, "module", e);

    task = ti_task_get_task(query->change, ti.thing0);
    if (!task || ti_task_add_deploy_module(task, module, NULL))
        ex_set_mem(e);  /* task cleanup is not required */
    else
        (void) ti_module_deploy(module, NULL, 0);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();
    return e->nr;
}
