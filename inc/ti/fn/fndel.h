#include <ti/fn/fn.h>

static int do__f_del(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    cleri_node_t * name_nd;
    ti_task_t * task;
    ti_thing_t * thing;

    if (!ti_val_is_object(query->rval))
        return fn_call_try("del", query, nd, e);

    if (fn_nargs("del", DOC_THING_DEL, 1, nargs, e))
        return e->nr;

    /*
     * This is a check for `iteration`.
     *
     * // without lock it breaks even with normal variable, luckily map puts
     * // a lock.
     * tmp.map(|| {
     *     tmp.del('x');
     * }
     */
    if (ti_val_try_lock(query->rval, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    name_nd = nd->children->node;

    if (ti_do_statement(query, name_nd, e) ||
        fn_arg_str("del", DOC_THING_DEL, 1, query->rval, e) ||
        ti_thing_o_del_e(thing, (ti_raw_t *) query->rval, e))
        goto unlock;

    if (thing->id)
    {
        task = ti_task_get_task(query->ev, thing, e);
        if (!task)
            goto unlock;

        if (ti_task_add_del(task, (ti_raw_t *) query->rval))
        {
            ex_set_mem(e);
            goto unlock;
        }
    }

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

unlock:
    ti_val_unlock((ti_val_t *) thing, true  /* lock was set */);
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}
