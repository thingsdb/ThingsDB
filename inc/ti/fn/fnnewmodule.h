#include <ti/fn/fn.h>

static int do__f_new_module(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * name, * file;
    ti_module_t * module;
    ti_task_t * task;
    ti_pkg_t * pkg;
    uint64_t * scope_id;
    cleri_children_t * child;

    if (fn_not_thingsdb_scope("new_module", query, e) ||
        fn_nargs("new_module", DOC_NEW_MODULE, 4, nargs, e) ||
        ti_do_statement(query, (child = nd->children)->node, e) ||
        fn_arg_str_slow("new_module", DOC_NEW_MODULE, 1, query->rval, e))
        return e->nr;

    name = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (!ti_name_is_valid_strn((const char *) name->data, name->n))
    {
        ex_set(e, EX_VALUE_ERROR,
                "module name must follow the naming rules"DOC_NAMES);
        goto fail0;
    }

    if (ti_do_statement(query, (child = child->next->next)->node, e) ||
        fn_arg_str_slow("new_module", DOC_NEW_MODULE, 2, query->rval, e))
        goto fail0;

    file = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (!strx_is_printablen((const char *) file->data, file->n))
    {
        ex_set(e, EX_VALUE_ERROR,
                "binary contains illegal characters"DOC_NEW_MODULE);
        goto fail1;
    }

    if (ti_do_statement(query, (child = child->next->next)->node, e))
        goto fail1;

    if (ti_val_is_nil(query->rval))
    {
        pkg = NULL;
    }
    else
    {
        pkg = ti_module_conf_pkg(query->rval);
        if (!pkg)
        {
            ex_set_mem(e);
            goto fail1;
        }
    }
    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, (child = child->next->next)->node, e))
        goto fail2;

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
            goto fail3;
    }
    else
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `new_module` expects argument 4 to be of "
            "type `"TI_VAL_STR_S"` or `"TI_VAL_NIL_S"` "
            "but got type `%s` instead"DOC_NEW_MODULE,
            ti_val_str(query->rval));
        goto fail2;
    }

    if (ti_modules_by_raw(name))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "module `%.*s` already exists",
                (int) name->n,
                (const char *) name->data);
        goto fail3;
    }

    module = ti_module_create(
            (const char *) name->data,
            name->n,
            (const char *) file->data,
            file->n,
            util_now_tsec(),
            pkg,
            scope_id);
    if (!module)
        goto fail3;

    pkg = NULL;
    scope_id = NULL;

    task = ti_task_get_task(query->ev, ti.thing0);
    if (!task || ti_task_add_new_module(task, module))
    {
        ti_module_destroy(smap_pop(ti.modules, module->name->str));
        ex_set_mem(e);
    }
    else
        ti_module_load(module);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

fail3:
    free(scope_id);
fail2:
    free(pkg);
fail1:
    ti_val_unsafe_drop((ti_val_t *) file);
fail0:
    ti_val_unsafe_drop((ti_val_t *) name);
    return e->nr;
}
