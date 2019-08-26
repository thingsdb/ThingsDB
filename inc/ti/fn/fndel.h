#include <ti/fn/fn.h>

#define DEL_DOC_ TI_SEE_DOC("#del")

static int do__f_del(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    cleri_node_t * name_nd;
    ti_task_t * task;
    ti_thing_t * thing;

    if (fn_not_chained("del", query, e))
        return e->nr;

    if (!ti_val_is_thing(query->rval))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `del`"DEL_DOC_,
                ti_val_str(query->rval));
        return e->nr;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `del` takes 1 argument but %d were given"DEL_DOC_,
                nargs);
        return e->nr;
    }

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

    if (ti_do_scope(query, name_nd, e))
        goto unlock;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `del` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead"DEL_DOC_,
                ti_val_str(query->rval));
        goto unlock;
    }

    if (ti_thing_del_e(thing, (ti_raw_t *) query->rval, e))
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
