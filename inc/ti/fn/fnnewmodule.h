#include <ti/fn/fn.h>

static int do__f_new_module(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * name, * binary;
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

    binary = (ti_raw_t *) query->rval;
    query->rval = NULL;

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
        scope_id = malloc(sizeof(uint64_t));
        if (!scope_id)
            goto fail2;

        if (query->collection)
            *scope_id = query->collection->root->id;
        else if (query->qbind.flags & TI_QBIND_FLAG_THINGSDB)
            *scope_id = TI_SCOPE_THINGSDB;
        else if (query->qbind.flags & TI_QBIND_FLAG_NODE)
            *scope_id = TI_SCOPE_NODE;
        else
        {
            log_error("unknown query scope");
            assert(0);
        }
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
            (const char *) binary->data,
            binary->n,
            util_now_tsec(),
            pkg,
            scope_id);
    if (!module)
        goto fail3;

    pkg = NULL;
    scope_id = NULL;

    task = ti_task_get_task(query->ev, ti.thing0);

    if (!task || ti_task_add_new_module(task, module))
        ex_set_mem(e);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

fail3:
    free(scope_id);
fail2:
    free(pkg);
fail1:
    ti_val_unsafe_drop((ti_val_t *) binary);
fail0:
    ti_val_unsafe_drop((ti_val_t *) name);
    return e->nr;
}
