#include <ti/fn/fn.h>

static int do__f_deploy_module(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_module_t * module;
    ti_task_t * task;
    ti_raw_t * rname;
    ti_raw_t * mdata;

    if (fn_not_thingsdb_scope("deploy_module", query, e) ||
        fn_nargs("deploy_module", DOC_DEPLOY_MODULE, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str_slow("deploy_module", DOC_DEPLOY_MODULE, 1, query->rval, e))
        return e->nr;

    rname = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next->node, e) ||
        fn_arg_str_bytes_nil(
                "deploy_module", DOC_DEPLOY_MODULE, 2, query->rval, e))
        goto fail0;

    module = ti_modules_by_raw(rname);
    if (!module)
    {
        (void) ti_raw_err_not_found(rname, "module", e);
        goto fail0;
    }

    mdata = (ti_raw_t *) (ti_val_is_nil(query->rval) ? NULL : query->rval);

    switch (module->source_type)
    {
    case TI_MODULE_SOURCE_FILE:
        if (!ti_module_is_py(module) && ti_val_is_str(query->rval))
        {
            ex_set(e, EX_TYPE_ERROR,
                    "function `deploy_module` expects argument 2 to be of "
                    "type `"TI_VAL_BYTES_S"` or `"TI_VAL_NIL_S"`"
                    "but got type `"TI_VAL_STR_S"` instead; "
                    "type `"TI_VAL_STR_S"` is only allowed for Python modules"
                    DOC_DEPLOY_MODULE);
            goto fail0;
        }
        break;
    case TI_MODULE_SOURCE_GITHUB:
        if (ti_val_is_bytes(query->rval))
        {
            ex_set(e, EX_TYPE_ERROR,
                    "function `deploy_module` expects argument 2 to be of "
                    "type `"TI_VAL_STR_S"` or `"TI_VAL_NIL_S"`"
                    "but got type `"TI_VAL_BYTES_S"` instead; "
                    "type `"TI_VAL_BYTES_S"` is only allowed for file sources"
                    DOC_DEPLOY_MODULE);
            goto fail0;
        }

        if (mdata &&
            ti_modu_github_init(NULL, (const char *) mdata->data, mdata->n, e))
            goto fail0;

        break;
    }

    task = ti_task_get_task(query->change, ti.thing0);
    if (!task || ti_task_add_deploy_module(task, module, mdata))
        ex_set_mem(e);  /* task cleanup is not required */
    else
        (void) ti_module_deploy(
                module,
                mdata ? mdata->data : NULL,
                mdata ? mdata->n : 0);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

fail0:
    ti_val_unsafe_drop((ti_val_t *) rname);
    return e->nr;
}
