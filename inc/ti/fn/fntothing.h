#include <ti/fn/fn.h>

static int do__f_to_thing(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_task_t * task;
    ti_thing_t * thing;

    if (!ti_val_is_instance(query->rval))
        return fn_call_try("to_thing", query, nd, e);

    thing = (ti_thing_t *) query->rval;

    if (fn_nargs("to_thing", DOC_TYPED_TO_THING, 0, nargs, e) ||
        ti_type_use(thing->via.type, e) ||
        ti_query_test_thing_operation(query, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    query->rval = (ti_val_t *) ti_nil_get();

    if (thing->via.type->selfref || thing->via.type->refcount)
    {
        ex_set(e, EX_OPERATION,
                "conversion `to_thing` has failed; "
                "type `%s` is used as restriction and therefore ThingsDB is "
                "not able to guarantee the typed thing can be converted",
                thing->via.type->name);
        goto fail0;
    }

    ti_thing_t_to_object(thing);

    if (thing->id)
    {
        task = ti_task_get_task(query->change, thing);
        if (!task || ti_task_add_to_thing(task))
            ex_set_mem(e);
    }

fail0:
    ti_val_unlock((ti_val_t *) thing, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) thing);
    return e->nr;
}
