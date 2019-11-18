#include <ti/fn/fn.h>

static int do__f_set_type(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_type_t * type;
    ti_thing_t * thing;
    ti_task_t * task;
    size_t n;
    _Bool is_new_type = false;

    if (fn_not_collection_scope("set_type", query, e) ||
        fn_nargs("set_type", DOC_SET_TYPE, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("set_type", DOC_SET_TYPE, 1, query->rval, e) ||
        !(task = ti_task_get_task(query->ev, query->collection->root, e)))
        return e->nr;


    type = ti_types_by_raw(query->collection->types, (ti_raw_t *) query->rval);
    if (!type)
    {
        uint16_t type_id;
        ti_raw_t * rname = (ti_raw_t *) query->rval;

        if (!ti_name_is_valid_strn((const char *) rname->data, rname->n))
        {
            ex_set(e, EX_VALUE_ERROR,
                "function `new_type` expects "
                "argument 1 to be a valid type name"DOC_NAMES);
            return e->nr;
        }

        type_id = ti_types_get_new_id(query->collection->types, rname, e);
        if (e->nr)
            return e->nr;

        type = ti_type_create(
                query->collection->types,
                type_id,
                (const char *) rname->data,
                rname->n);

        if (!type)
        {
            ex_set_mem(e);
            return e->nr;
        }
        is_new_type = true;
    }
    else if (type->fields->n)
    {
        ex_set(e, EX_OPERATION_ERROR,
            "function `set_type` works only on a new type; "
            "use `mod_type()` if you want to change an existing type"
            DOC_MOD_TYPE);
        return e->nr;
    }

    if (ti_type_try_lock(type, e))
        goto fail0;

    ti_val_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next->node, e))
        goto fail1;

    if (!ti_val_is_thing(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `set_type` expects argument 2 to be of "
            "type `"TI_VAL_THING_S"` but got type `%s` instead"DOC_NEW_TYPE,
            ti_val_str(query->rval));
        goto fail1;
    }

    n = ti_query_count_type(query, type);
    if (n)
    {
        ex_set(e, EX_OPERATION_ERROR,
            "function `set_type` can only be used on a type without active "
            "instances; %zu active instance%s of type `%s` %s been found"
            DOC_SET_TYPE,
            n, n == 1 ? "" : "s", type->name, n == 1 ? "has" : "have");
        goto fail1;
    }

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_type_init_from_thing(type, thing, e))
        goto fail2;

    if ((is_new_type && ti_task_add_new_type(task, type)) ||
        ti_task_add_set_type(task, type))
    {
        ex_set_mem(e);
        goto fail2;
    }

    is_new_type = false;  /* set always to false to prevent cleanup */
    query->rval = (ti_val_t *) ti_nil_get();
    ti_type_map_cleanup(type);

fail2:
    ti_val_drop((ti_val_t *) thing);
fail1:
    ti_type_unlock(type, true /* lock is set for sure */);
fail0:
    if (is_new_type)
        ti_type_drop(type);

    return e->nr;
}
