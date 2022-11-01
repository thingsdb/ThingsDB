#include <ti/fn/fn.h>


static int do__f_assign(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    cleri_node_t * name_nd;
    ti_task_t * task = NULL;
    ti_thing_t * thing, * tsrc;

    if (!ti_val_is_thing(query->rval))
        return fn_call_try("assign", query, nd, e);

    if (fn_nargs("assign", DOC_THING_ASSIGN, 1, nargs, e))
        return e->nr;

    /*
     * This is a check for `iteration`.
     *
     * // without lock it breaks even with normal variable, luckily map puts
     * // a lock.
     * tmp.map(|| {
     *     tmp.assign({x: 1});
     * }
     */
    if (ti_query_test_thing_operation(query, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    name_nd = nd->children;

    if (ti_do_statement(query, name_nd, e) ||
        fn_arg_thing("assign", DOC_THING_ASSIGN, 1, query->rval, e))
    {
        goto fail0;
    }

    tsrc = (ti_thing_t *) query->rval;
    query->rval = (ti_val_t *) thing;
    ti_incref(thing);

    if (thing->id && !(task = ti_task_get_task(query->change, thing)))
    {
        ex_set_mem(e);
        goto fail1;
    }

    (void) ti_thing_assign(thing, tsrc, task, e);

fail1:
    ti_val_unsafe_drop((ti_val_t *) tsrc);
fail0:
    ti_val_unlock((ti_val_t *) thing, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) thing);
    return e->nr;
}
