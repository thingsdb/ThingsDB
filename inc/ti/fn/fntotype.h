#include <ti/fn/fn.h>

static int do__f_to_type(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_task_t * task;
    ti_thing_t * thing;
    ti_raw_t * rname;
    ti_type_t * type;

    if (!ti_val_is_object(query->rval))
        return fn_call_try("to_type", query, nd, e);

    if (query->collection &&
        query->collection->commits &&
        query->collection->root == (ti_thing_t *) query->rval &&
        !query->commit)
    {
        ex_set(e, EX_OPERATION,
            "function `to_type` requires a commit before "
            "it can be used on the `root()` of a `%s` scope "
            "with commit history enabled"DOC_COMMIT,
             ti_query_scope_name(query));
        return e->nr;
    }

    if (fn_nargs("to_type", DOC_THING_TO_TYPE, 1, nargs, e) ||
        ti_query_test_thing_operation(query, e))
        return e->nr;

    /*
     * This is a check for `iteration`.
     *
     * // without lock it breaks even with normal variable, luckily map puts
     * // a lock.
     * tmp.map(|| {
     *     tmp.to_type('X');
     * }
     */
    if (ti_val_try_lock(query->rval, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e) ||
        fn_arg_str("to_type", DOC_THING_TO_TYPE, 1, query->rval, e))
        goto fail0;

    if (ti_thing_o_is_restricted(thing))
    {
        ex_set(e, EX_VALUE_ERROR,
                "conversion failed; "
                "the thing you try to convert has a value restriction");
        goto fail0;
    }

    rname = (ti_raw_t *) query->rval;
    query->rval = (ti_val_t *) ti_nil_get();

    type = query->collection
       ? ti_types_by_raw(query->collection->types, rname)
       : NULL;

    if (!type)
    {
        (void) ti_raw_err_not_found(rname, "type", e);
        goto fail1;
    }

    if (ti_type_wrap_only_e(type, e) ||
        ti_type_use(type, e) ||
        ti_type_convert(type, thing, query->change, e))
        goto fail1;

    if (thing->id)
    {
        task = ti_task_get_task(query->change, thing);
        if (!task || ti_task_add_to_type(task, type))
            ex_set_mem(e);
    }

fail1:
    ti_val_unsafe_drop((ti_val_t *) rname);

fail0:
    ti_val_unlock((ti_val_t *) thing, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) thing);
    return e->nr;
}
