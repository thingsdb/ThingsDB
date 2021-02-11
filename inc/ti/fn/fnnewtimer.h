#include <ti/fn/fn.h>

static int do__f_new_timer(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    int rc;
    cleri_children_t * child;
    ti_name_t * name;
    ti_task_t * task;
    ti_procedure_t * procedure;
    ti_closure_t * closure;
    time_t * start_ts;
    uint32_t repeat;

    if (fn_not_thingsdb_or_collection_scope("new_timer", query, e) ||
        fn_nargs_range("new_timer", DOC_NEW_TIMER, 4, 5, nargs, e) ||
        ti_do_statement(query, (child = nd->children)->node, e))
        return e->nr;

    if (ti_val_is_str(query->rval))
    {
        ti_raw_t * raw = (ti_raw_t *) query->rval;
        if (!ti_name_is_valid_strn((const char *) raw->data, raw->n))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "timer name must follow the naming rules"DOC_NAMES);
            return e->nr;
        }

        name = ti_names_from_raw((ti_raw_t *) query->rval);
        if (!name)
        {
            ex_set_mem(e);
            return e->nr;
        }
    }
    else if (ti_val_is_nil(query->rval))
    {
        name = NULL;    /* anonymous timer */
    }
    else
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `new_timer` expects argument 1 to be to be of "
            "type `"TI_VAL_STR_S"` or `"TI_VAL_NIL_S"` "
            "but got type `%s` instead"DOC_NEW_TIMER,
            ti_val_str(query->rval));
        return e->nr;
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, (child = child->next->next)->node, e) ||
        fn_arg_datetime("new_timer", DOC_NEW_TIMER, 2, query->rval, e))
        goto fail0;

    start_ts = DATETIME(query->rval);
    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, (child = child->next->next)->node, e))
        goto fail0;

    if (ti_val_is_int(query->rval))
    {
        if (VINT(query->rval) < 0 || VINT(query->rval) > UINT32_MAX)
        {
            ex_set(e, EX_VALUE_ERROR,
                    "repeat value out-or-range"DOC_NEW_TIMER);
            goto fail0;
        }
        repeat = (uint32_t) VINT(query->rval);
    }
    else if (ti_val_is_nil(query->rval))
    {
        repeat = 0;    /* no repeat */
    }
    else
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `new_timer` expects argument 3 to be to be of "
            "type `"TI_VAL_INT_S"` or `"TI_VAL_NIL_S"` "
            "but got type `%s` instead"DOC_NEW_TIMER,
            ti_val_str(query->rval));
        goto fail0;
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, (child = child->next->next)->node, e) ||
        fn_arg_closure("new_timer", DOC_NEW_TIMER, 4, query->rval, e))
        goto fail0;

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (ti_closure_unbound(closure, e))
        goto fail1;

    ti_next_thing_id()

    timer = ti_timer_create(
            name,
            raw->n,
            closure,
            util_now_tsec());
    if (!procedure)
        goto alloc_error;

    rc = ti_procedures_add(procedures, procedure);
    if (rc < 0)
        goto alloc_error;
    if (rc > 0)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "procedure `%s` already exists",
                procedure->name);
        goto fail2;
    }

    task = ti_task_get_task(
            query->ev,
            query->collection ? query->collection->root : ti.thing0);
    if (!task || ti_task_add_new_procedure(task, procedure))
        goto undo;

    query->rval = (ti_val_t *) raw;
    ti_incref(query->rval);

    goto done;

undo:
    (void) ti_procedures_pop(procedures, procedure);

alloc_error:
    if (!e->nr)
        ex_set_mem(e);

fail2:
    ti_procedure_destroy(procedure);

done:
fail1:
    ti_val_unsafe_drop((ti_val_t *) closure);

fail0:
    ti_name_drop(name);
    return e->nr;
}
