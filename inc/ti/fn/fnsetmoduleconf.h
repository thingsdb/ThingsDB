#include <ti/fn/fn.h>

static int do__f_set_module_conf(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_module_t * module;
    ti_task_t * task;
    ti_raw_t * rname;
    ti_pkg_t * pkg;

    if (fn_not_thingsdb_scope("set_module_conf", query, e) ||
        fn_nargs("set_module_conf", DOC_SET_MODULE_CONF, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str_slow(
                "set_module_conf", DOC_SET_MODULE_CONF, 1, query->rval, e))
        return e->nr;

    rname = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next->node, e))
        goto fail0;

    /* All statements are parsed, now check if the module (still) exists) */
    module = ti_modules_by_raw(rname);
    if (!module)
    {
        (void) ti_raw_err_not_found(rname, "module", e);
        goto fail0;
    }

    if (ti_val_is_nil(query->rval))
    {
        pkg = NULL;
    }
    else
    {
        pkg = ti_module_conf_pkg(query->rval, query);
        if (!pkg)
        {
            ex_set_mem(e);
            goto fail0;
        }
    }

    free(module->conf_pkg);
    module->conf_pkg = pkg;
    pkg = NULL;

    ti_module_update_conf(module);

    task = ti_task_get_task(query->change, ti.thing0);
    if (!task || ti_task_add_set_module_conf(task, module))
        ex_set_mem(e);  /* task cleanup is not required */

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

fail0:
    ti_val_unsafe_drop((ti_val_t *) rname);
    return e->nr;
}
