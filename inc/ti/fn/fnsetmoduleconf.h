#include <ti/fn/fn.h>

static int do__f_set_module_conf(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_module_t * module;
    ti_task_t * task;

    if (fn_not_thingsdb_scope("set_module_conf", query, e) ||
        fn_nargs("set_module_conf", DOC_SET_MODULE_CONF, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str_slow(
                "set_module_conf", DOC_SET_MODULE_CONF, 1, query->rval, e))
        return e->nr;

    module = ti_modules_by_raw((ti_raw_t *) query->rval);
    if (!module)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "module", e);

    task = ti_task_get_task(query->ev, ti.thing0);
    if (!task || ti_task_add_set_module_conf(task, module))
        ex_set_mem(e);  /* task cleanup is not required */

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

done:
    return e->nr;
}
