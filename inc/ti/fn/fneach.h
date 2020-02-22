#include <ti/fn/fn.h>

typedef struct
{
    ex_t * e;
    ti_closure_t * closure;
    ti_query_t * query;
} each__walk_t;

static int each__walk_set(ti_thing_t * t, each__walk_t * w)
{
    if (ti_closure_vars_val_idx(w->closure, (ti_val_t *) t, t->id) ||
        ti_closure_do_statement(w->closure, w->query, w->e))
        return -1;
    w->query->rval = NULL;
    return 0;
}

static int do__f_each(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const char * doc;
    const int nargs = langdef_nd_n_function_params(nd);
    ti_closure_t * closure;
    ti_val_t * iterval;
    int lock_was_set;

    doc = doc_each(query->rval);
    if (!doc)
        return fn_call_try("each", query, nd, e);

    if (fn_nargs("each", doc, 1, nargs, e))
        return e->nr;

    lock_was_set = ti_val_ensure_lock(query->rval);
    iterval = query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e) ||
        fn_arg_closure("each", doc, 1, query->rval, e))
        goto fail0;

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (ti_closure_try_wse(closure, query, e) ||
        ti_closure_inc(closure, query, e))
        goto fail1;

    switch (iterval->tp)
    {
    case TI_VAL_THING:
    {
        ti_thing_t * thing = (ti_thing_t *) iterval;
        if (ti_thing_is_object(thing))
        {
            for (vec_each(thing->items, ti_prop_t, p))
            {
                if (ti_closure_vars_prop(closure, p, e) ||
                    ti_closure_do_statement(closure, query, e))
                    goto fail2;
                query->rval = NULL;
            }
        }
        else
        {
            ti_name_t * name;
            ti_val_t * val;
            for (thing_each(thing, name, val))
            {
                if (ti_closure_vars_nameval(closure, name, val, e) ||
                    ti_closure_do_statement(closure, query, e))
                    goto fail2;
                query->rval = NULL;
            }
        }
        break;
    }
    case TI_VAL_ARR:
    {
        int64_t idx = 0;
        for (vec_each(((ti_varr_t *) iterval)->vec, ti_val_t, v), ++idx)
        {
            if (ti_closure_vars_val_idx(closure, v, idx) ||
                ti_closure_do_statement(closure, query, e))
                goto fail2;

            query->rval = NULL;
        }
        break;
    }
    case TI_VAL_SET:
    {
        each__walk_t w = {
                .e = e,
                .closure = closure,
                .query = query,
        };
        if (imap_walk(
                ((ti_vset_t *) iterval)->imap,
                (imap_cb) each__walk_set,
                &w))
        {
            if (!e->nr)
                ex_set_mem(e);
            goto fail2;
        }
        break;
    }
    }

    assert (query->rval == NULL);
    query->rval = (ti_val_t *) ti_nil_get();

fail2:
    ti_closure_dec(closure, query);

fail1:
    ti_val_drop((ti_val_t *) closure);

fail0:
    ti_val_unlock(iterval, lock_was_set);
    ti_val_drop(iterval);
    return e->nr;
}
