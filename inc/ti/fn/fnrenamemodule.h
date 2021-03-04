#include <ti/fn/fn.h>

static int do__f_rename_module(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_task_t * task;
    ti_module_t * module;
    ti_raw_t * nname;

    if (fn_not_thingsdb_scope("rename_module", query, e) ||
        fn_nargs("rename_module", DOC_RENAME_MODULE, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str_slow("rename_module", DOC_RENAME_MODULE, 1, query->rval, e))
        return e->nr;

    module = ti_modules_by_raw((ti_raw_t *) query->rval);
    if (!module)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "module", e);

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next->node, e) ||
        fn_arg_str_slow("rename_module", DOC_RENAME_MODULE, 2, query->rval, e))
        return e->nr;

    nname = (ti_raw_t *) query->rval;
    query->rval = (ti_val_t *) ti_nil_get();

    if (!ti_name_is_valid_strn((const char *) nname->data, nname->n))
    {
        ex_set(e, EX_VALUE_ERROR,
                "module name must follow the naming rules"DOC_NAMES);
        goto fail0;
    }

    if (ti_modules_by_raw(nname))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "module `%.*s` already exists",
                nname->n, (const char *) nname->data);
        goto fail0;
    }

    task = ti_task_get_task(query->ev, ti.thing0);
    if (!task ||
        ti_task_add_rename_module(task, module, nname) ||
        ti_modules_rename(module, (const char *) nname->data, nname->n))
        ex_set_mem(e);  /* task cleanup is not required */

fail0:
    ti_val_unsafe_drop((ti_val_t *) nname);
    return e->nr;

}
