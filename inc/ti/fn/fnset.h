#include <ti/fn/fn.h>

static int do__set_new_type(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);

    if (nargs == 1 && nd->children->next == NULL)
    {
        return (
            ti_do_statement(query, nd->children->node, e) ||
            ti_val_convert_to_set(&query->rval, e)
        );
    }

    if (nargs)
    {
        cleri_children_t * child = nd->children;
        ti_vset_t * vset = ti_vset_create();

        if (!vset)
        {
            ex_set_mem(e);
            return e->nr;
        }

        do
        {
            if (ti_do_statement(query, child->node, e) ||
                ti_vset_add_val(vset, query->rval, e) < 0)
            {
                ti_vset_destroy(vset);
                return e->nr;
            }

            ti_val_drop(query->rval);
            query->rval = NULL;
        }
        while (child->next && (child = child->next->next));

        query->rval = (ti_val_t *) vset;
        return e->nr;
    }

    assert (query->rval == NULL);
    query->rval = (ti_val_t *) ti_vset_create();
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}

static int do__set_property(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_prop_t * prop;
    ti_thing_t * thing;
    ti_name_t * name;
    ti_raw_t * rname;
    size_t max_props = query->collection
            ? query->collection->quota->max_props
            : TI_QUOTA_NOT_SET;     /* check for scope since assign is
                                       possible when chained in all scopes */

    if (!ti_val_is_thing(query->rval))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no function `set`"DOC_SET_PROPERTY,
                ti_val_str(query->rval));
        return e->nr;
    }

    if (fn_nargs("set", DOC_SET_PROPERTY, 2, nargs, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (thing->items->n == max_props)
    {
        ex_set(e, EX_MAX_QUOTA,
            "maximum properties quota of %zu has been reached"
            TI_SEE_DOC("#quotas"), max_props);
        goto fail0;
    }

    if (ti_do_statement(query, nd->children->node, e))
        goto fail0;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `set` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"DOC_SET_PROPERTY,
            ti_val_str(query->rval));
        goto fail0;
    }

    rname = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next->node, e))
        goto fail1;


    if (ti_val_make_assignable(&query->rval, e))
        goto fail1;

    name = ti_names_get((const char *) rname->data, rname->n);
    if (!name)
    {
        ex_set_mem(e);
        goto fail1;
    }

    prop = ti_thing_o_prop_set_e(thing, name, query->rval, e);
    if (!prop)
    {
        assert (e->nr);
        ti_name_drop(name);
        goto fail1;
    }

    ti_incref(prop->val);

    if (thing->id)
    {
        ti_task_t * task = ti_task_get_task(query->ev, thing, e);
        if (!task)
            goto fail1;

        if (ti_task_add_set(task, prop->name, prop->val))
        {
            ex_set_mem(e);
            goto fail1;
        }
        ti_chain_set(&query->chain, thing, prop->name);
    }

fail1:
    ti_val_drop((ti_val_t *) rname);
fail0:
    ti_val_unlock((ti_val_t *) thing, true /* lock_was_set */);
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}

static int do__f_set(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    return query->rval
            ? do__set_property(query, nd, e)
            : do__set_new_type(query, nd, e);
}
