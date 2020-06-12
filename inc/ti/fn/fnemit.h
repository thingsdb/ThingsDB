#include <ti/fn/fn.h>


static int do__f_emit(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_thing_t * thing;
    ti_raw_t * revent;
    vec_t * vec = NULL;

    if (!ti_val_is_thing(query->rval))
        return fn_call_try("emit", query, nd, e);

    if (fn_nargs_min("emit", DOC_THING_EMIT, 1, nargs, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("emit", DOC_THING_EMIT, 1, query->rval, e))
        goto fail0;

    revent = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (nargs > 1)
    {
        cleri_children_t * child = nd->children->next->next;

        vec = vec_new(nargs-1);
        if (!vec)
        {
            ex_set_mem(e);
            goto fail1;
        }

        do
        {
            if (ti_do_statement(query, child->node, e))
                goto fail2;

            VEC_push(vec, query->rval);
            query->rval = NULL;
        }
        while (child->next && (child = child->next->next));

    }

    if (thing->id)
    {
        ti_task_t * task = ti_task_get_task(query->ev, thing, e);
        if (!task)
            goto fail2;

        if (ti_task_add_event(task, revent, vec))
            ex_set_mem(e);
    }

    query->rval = (ti_val_t *) ti_nil_get();

fail2:
    vec_destroy(vec, (vec_destroy_cb) ti_val_drop);
fail1:
    ti_val_drop((ti_val_t *) revent);
fail0:
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}
