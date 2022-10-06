#include <ti/fn/fn.h>

static int do__f_ren(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_task_t * task;
    ti_thing_t * thing;
    ti_raw_t * oname, * nname;

    if (!ti_val_is_object(query->rval))
        return fn_call_try("ren", query, nd, e);

    if (fn_nargs("ren", DOC_THING_REN, 2, nargs, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e) ||
        fn_arg_str("ren", DOC_THING_REN, 1, query->rval, e))
        goto fail0;
    oname = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next, e) ||
        fn_arg_str("ren", DOC_THING_REN, 2, query->rval, e))
        goto fail1;
    nname = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_thing_o_ren(
            thing,
            (const char *) oname->data,
            oname->n,
            (const char *) nname->data,
            nname->n,
            e))
        goto fail2;

    query->rval = (ti_val_t *) ti_nil_get();

    if (thing->id)
    {
        task = ti_task_get_task(query->change, thing);
        if (!task || ti_task_add_ren(task, oname, nname))
            ti_panic("failed to create rename task");
    }

fail2:
    ti_val_unsafe_drop((ti_val_t *) nname);
fail1:
    ti_val_unsafe_drop((ti_val_t *) oname);
fail0:
    ti_val_unlock((ti_val_t *) thing, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) thing);
    return e->nr;
}
