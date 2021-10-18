#include <ti/fn/fn.h>

static int do__f_task(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    vec_t ** tasks = ti_query_tasks(query);
    cleri_children_t * child;
    ti_vtask_t * vtask;

    if (fn_not_thingsdb_or_collection_scope("task", query, e) ||
        fn_nargs_range("task", DOC_TASK, 1, 3, nargs, e) ||
        ti_do_statement(query, (child = nd->children)->node, e))
        return e->nr;

    if (nargs == 1)
    {
        int64_t task_id;
        if (!ti_val_is_int(query->rval))
        {
            ex_set(e, EX_TYPE_ERROR,
                    "cannot convert type `%s` to `"TI_VAL_TASK_S"`",
                    ti_val_str(query->rval));
            return e->nr;
        }

        task_id = VINT(query->rval);
        vtask = ti_vtask_by_id(*tasks, task_id);
        if (!vtask)
        {
            ex_set(e, EX_LOOKUP_ERROR, "task Id %"PRId64" not found", task_id);
            return e->nr;
        }
        ti_incref(vtask);
        ti_val_unsafe_drop(query->rval);
        query->rval = (ti_val_t *) vtask;
        return e->nr;
    }
    else
    {
        time_t run_at, now = util_now_tsec();
        ti_closure_t * closure;
        vec_t * args;
        ti_task_t * task;
        size_t m, n;

        if (fn_arg_datetime("task", DOC_TASK, 1, query->rval, e))
            return e->nr;

        run_at = DATETIME(query->rval);
        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;

        if ((*tasks)->n >= TI_MAX_TASK_COUNT)
        {
            ex_set(e, EX_MAX_QUOTA,
                    "maximum number of tasks (%u) is reached",
                    TI_MAX_TASK_COUNT);
            return e->nr;
        }

        if (run_at < now || run_at > UINT32_MAX)
        {
            ex_set(e, EX_VALUE_ERROR, "start time out-of-range"DOC_TASK);
            return e->nr;
        }

        if (nargs < 2)
        {
            ex_set(e, EX_NUM_ARGUMENTS,
                    "function `task` requires at least a closure argument to "
                    "create a new task"DOC_TASK);
            return e->nr;
        }

        if (ti_do_statement(query, (child = child->next->next)->node, e) ||
            fn_arg_closure("task", DOC_TASK, 2, query->rval, e))
            return e->nr;

        closure = (ti_closure_t *) query->rval;
        query->rval = NULL;

        if (ti_closure_unbound(closure, e))
            goto fail1;

        m = closure->vars->n;

        if (!(args = vec_new(m)))
        {
            ex_set_mem(e);
            goto fail2;
        }

        if (args->n)
            VEC_push(args, ti_nil_get());

        if (nargs == 3)
        {
            if (ti_do_statement(query, (child = child->next->next)->node, e) ||
                fn_arg_array("task", DOC_TASK, 3, query->rval, e))
                goto fail2;

            n = VARR(query->rval)->n;
            if (n >= m)
            {
                ex_set(e, EX_NUM_ARGUMENTS,
                        "got %zu task argument%s while the given closure "
                        "accepts no more than %zu argument%s "
                        "(first closure argument will be the task)"DOC_TASK,
                        n, n == 1 ? "" : "s",
                        m ? m-1 : 0, m == 2 ? "" : "s");
                goto fail2;
            }

            if (!query->collection &&
                ti_timer_check_thingsdb_args(VARR(query->rval), e))
                goto fail2;

            for (vec_each(VARR(query->rval), ti_val_t, v), --m)
            {
                VEC_push(args, v);
                ti_incref(v);
            }

            ti_val_unsafe_drop(query->rval);
            query->rval = NULL;
        }

        while (m--)
            VEC_push(args, ti_nil_get());

        vtask = ti_vtask_create(
                ti_next_free_id(),
                (uint64_t) run_at,
                query->user,
                closure,
                NULL,
                args);

        if (!vtask || vec_push(tasks, vtask))
        {
            ex_set_mem(e);
            goto fail3;
        }

        task = ti_task_get_task(
                query->change,
                query->collection ? query->collection->root : ti.thing0);

        if (!task || ti_task_add_new_vtask(task, vtask))
            goto undo;

        query->rval = (ti_val_t *) vtask;
        goto done;

    undo:
        ex_set_mem(e);
        VEC_pop(*tasks);
    fail3:
        ti_vtask_drop(vtask);
    fail2:
        vec_destroy(args, (vec_destroy_cb) ti_val_drop);
    done:
    fail1:
        ti_val_unsafe_drop((ti_val_t *) closure);
    }
    return e->nr;
}
