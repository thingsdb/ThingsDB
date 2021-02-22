#include <ti/fn/fn.h>

static int do__f_set_module_scope(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_module_t * module;
    ti_task_t * task;
    ti_raw_t * rname;
    uint64_t * scope_id;

    if (fn_not_thingsdb_scope("set_module_scope", query, e) ||
        fn_nargs("set_module_scope", DOC_SET_MODULE_SCOPE, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str_slow(
                "set_module_scope", DOC_SET_MODULE_SCOPE, 1, query->rval, e))
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
        scope_id = NULL;
    }
    else if (ti_val_is_str(query->rval))
    {
        ti_scope_t scope;
        ti_raw_t * rscope = (ti_raw_t *) query->rval;
        scope_id = malloc(sizeof(uint64_t));
        if (!scope_id ||
            ti_scope_init(&scope, (const char *) rscope->data, rscope->n, e) ||
            ti_scope_id(&scope, scope_id, e))
            goto fail1;
    }
    else
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `set_module_scope` expects argument 2 to be of "
            "type `"TI_VAL_STR_S"` or `"TI_VAL_NIL_S"` "
            "but got type `%s` instead"DOC_SET_MODULE_SCOPE,
            ti_val_str(query->rval));
        goto fail0;
    }

    free(module->scope_id);
    module->scope_id = scope_id;
    scope_id = NULL;

    task = ti_task_get_task(query->ev, ti.thing0);
    if (!task || ti_task_add_set_module_scope(task, module))
        ex_set_mem(e);  /* task cleanup is not required */

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

fail1:
    free(scope_id);
fail0:
    ti_val_unsafe_drop((ti_val_t *) rname);
    return e->nr;
}
