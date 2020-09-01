#include <ti/fn/fn.h>

static int do__f_del(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    cleri_node_t * name_nd;
    ti_task_t * task;
    ti_thing_t * thing;
    ti_prop_t * prop;
    ti_raw_t * rname;

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
        fn_arg_str("del", DOC_THING_DEL, 1, query->rval, e))
        goto fail0;

    rname = (ti_raw_t *) query->rval;

    prop = ti_thing_o_del_e(thing, rname, e);
    if (!prop)
        goto fail0;  /* error is set, rname is still bound to query */

    query->rval = prop->val;
    ti_incref(query->rval);

    ti_val_attach(query->rval, NULL, NULL);

    ti_prop_destroy(prop);

    if (thing->id)
    {
        task = ti_task_get_task(query->ev, thing);
        if (!task || ti_task_add_del(task, rname))
        {
            ex_set_mem(e);
            goto fail1;
        }
    }
    else
        ti_thing_may_push_gc((ti_thing_t *) query->rval);

fail1:
    ti_val_unsafe_drop((ti_val_t *) rname);

fail0:
    ti_val_unlock((ti_val_t *) thing, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) thing);
    return e->nr;
}
