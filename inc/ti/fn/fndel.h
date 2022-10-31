#include <ti/fn/fn.h>

static int do__f_del_vtask(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_task_t * task;
    ti_vtask_t * vtask;

    if (!query->change)
    {
        ex_set(e, EX_OPERATION,
                "operation on a task; "
                "%s(...)` to enforce a change",
                (query->qbind.flags & TI_QBIND_FLAG_NSE)
                    ? "remove `nse"
                    : "use `wse");
        return e->nr;
    }

    vtask = (ti_vtask_t *) query->rval;
    if (ti_vtask_is_locked(vtask, e))
        return e->nr;

    query->rval = (ti_val_t *) ti_nil_get();;

    if (fn_nargs("del", DOC_TASK_DEL, 0, nargs, e))
        goto fail0;

    if (vtask->id)
    {
        task = ti_task_get_task(
                query->change,
                query->collection ? query->collection->root : ti.thing0);

        if (task && ti_task_add_vtask_del(task, vtask) == 0)
            /* must be after creating the task or the task Id is lost */
            ti_vtask_del(vtask->id, query->collection);
        else
            ex_set_mem(e);
    }

fail0:
    ti_vtask_unsafe_drop(vtask);
    return e->nr;
}

static int do__f_del_thing(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_task_t * task;
    ti_thing_t * thing;
    ti_item_t * item;
    ti_raw_t * rname;

    if (!ti_val_is_object(query->rval))
        return fn_call_try("del", query, nd, e);

    if (fn_nargs("del", DOC_THING_DEL, 1, nargs, e) ||
        ti_query_test_thing_operation(query, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e) ||
        fn_arg_str("del", DOC_THING_DEL, 1, query->rval, e))
        goto fail0;

    rname = (ti_raw_t *) query->rval;

    item = ti_thing_o_del_e(thing, rname, e);
    if (!item)
        goto fail0;  /* error is set, rname is still bound to query */

    query->rval = item->val;
    ti_incref(query->rval);

    ti_item_unassign_destroy(item);

    if (thing->id)
    {
        task = ti_task_get_task(query->change, thing);
        if (!task || ti_task_add_del(task, rname))
        {
            ex_set_mem(e);
            goto fail1;
        }
    }

fail1:
    ti_val_unsafe_drop((ti_val_t *) rname);

fail0:
    ti_val_unlock((ti_val_t *) thing, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) thing);
    return e->nr;
}

static int do__f_del(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    return ti_val_is_thing(query->rval)
            ? do__f_del_thing(query, nd, e)
            : ti_val_is_task(query->rval)
            ? do__f_del_vtask(query, nd, e)
            : fn_call_try("del", query, nd, e);
}
