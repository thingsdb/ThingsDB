#include <ti/fn/fn.h>

static int do__f_set_new(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    if (nargs == 1 && nd->children->next == NULL)
    {
        return (
            ti_do_statement(query, nd->children, e) ||
            ti_val_convert_to_set(&query->rval, e)
        );
    }

    if (nargs)
    {
        cleri_node_t * child = nd->children;
        ti_vset_t * vset = ti_vset_create();

        if (!vset)
        {
            ex_set_mem(e);
            return e->nr;
        }

        do
        {
            if (ti_do_statement(query, child, e) ||
                ti_vset_add_val(vset, query->rval, e) < 0)
            {
                ti_vset_destroy(vset);
                return e->nr;
            }

            ti_val_unsafe_drop(query->rval);
            query->rval = NULL;
        }
        while (child->next && (child = child->next->next));

        query->rval = (ti_val_t *) vset;
        return e->nr;
    }

    assert(query->rval == NULL);
    query->rval = (ti_val_t *) ti_vset_create();
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}

static int do__f_set_thing(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_witem_t witem;
    ti_thing_t * thing;
    ti_raw_t * rname;

    if (!ti_val_is_thing(query->rval))
        return fn_call_try("set", query, nd, e);

    if (fn_nargs("set", DOC_THING_SET, 2, nargs, e) ||
        ti_query_test_thing_operation(query, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e) ||
        fn_arg_str("set", DOC_THING_SET, 1, query->rval, e))
        goto fail0;

    rname = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next, e))
        goto fail1;

    if (ti_thing_set_val_from_strn(
            &witem,
            thing,
            (const char *) rname->data,
            rname->n,
            &query->rval, e))
        goto fail1;

    if (thing->id)
    {
        ti_task_t * task = ti_task_get_task(query->change, thing);
        if (!task || ti_task_add_set(task, witem.key, *witem.val))
        {
            ex_set_mem(e);
            goto fail1;
        }
    }

fail1:
    ti_val_unsafe_drop((ti_val_t *) rname);
fail0:
    ti_val_unlock((ti_val_t *) thing, true /* lock_was_set */);
    ti_val_unsafe_drop((ti_val_t *) thing);
    return e->nr;
}

static inline int do__f_set(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    return query->rval
            ? do__f_set_thing(query, nd, e)
            : do__f_set_new(query, nd, e);
}
