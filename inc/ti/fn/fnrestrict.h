#include <ti/fn/fn.h>

static int restrict__cb(ti_prop_t * prop, uint16_t * spec)
{
    return !ti_val_is_spec(prop->val, *spec);
}

static int do__f_restrict(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_task_t * task;
    ti_thing_t * thing;
    uint16_t spec = TI_SPEC_ANY;

    if (!ti_val_is_object(query->rval))
        return fn_call_try("restrict", query, nd, e);

    if (fn_nargs("restrict", DOC_THING_RESTRICT, 1, nargs, e))
        return e->nr;

    /*
     * Require a lock as the next statement might convert this thing into a
     * type.
     */
    if (ti_val_try_lock(query->rval, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str_nil("restrict", DOC_THING_RESTRICT, 1, query->rval, e))
        goto done;

    if (ti_val_is_str(query->rval) &&
        ti_spec_from_raw(&spec, (ti_raw_t *) query->rval, query->collection, e))
        goto done;

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) thing;
    ti_incref(thing);

    if (spec == thing->via.spec)
        goto done;

    if (thing->via.spec != TI_SPEC_ANY &&
            !ti_query_thing_can_change_spec(query, thing))
    {
        ex_set(e, EX_OPERATION,
                "current restriction is enforced by at least one type");
        goto done;
    }

    if (spec != TI_SPEC_ANY)
    {
        int rc = 0;

        if (ti_thing_is_dict(thing))
        {
            rc = (ti_spec_rval_enum) smap_values(
                    thing->items.smap,
                    (smap_val_cb) restrict__cb,
                    &spec);
        }
        else for (vec_each(thing->items.vec, ti_prop_t, prop))
            if ((rc = !ti_val_is_spec(prop->val, spec)))
                break;

        if (rc)
        {
            ex_set(e, EX_VALUE_ERROR,
                    "at least one of the existing values does not match the "
                    "desired restriction");
            goto done;
        }
    }

    if (thing->id)
    {
        task = ti_task_get_task(query->change, thing);
        if (!task || ti_task_add_restrict(task, spec))
        {
            ex_set_mem(e);
            goto done;
        }
    }

    thing->via.spec = spec;

done:
    ti_val_unlock((ti_val_t *) thing, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) thing);
    return e->nr;
}
