#include <ti/fn/fn.h>

static int do__f_del_module(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_module_t * module;
    ti_task_t * task;

    if (fn_not_thingsdb_scope("del_module", query, e) ||
        fn_nargs("del_module", DOC_DEL_MODULE, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_str_slow("del_module", DOC_DEL_MODULE, 1, query->rval, e))
        return e->nr;

    module = ti_modules_by_raw((ti_raw_t *) query->rval);
    if (!module)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "module", e);

    task = ti_task_get_task(query->change, ti.thing0);
    if (!task || ti_task_add_del_module(task, module))
        ex_set_mem(e);  /* task cleanup is not required */
    else
        /* this will remove the module so it cannot be used after here */
        ti_module_del(module, true);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
