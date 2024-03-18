#include <ti/fn/fn.h>

typedef struct
{
    ex_t * e;
    ti_closure_t * closure;
    ti_query_t * query;
} each__walk_t;

static int each__walk_set(ti_thing_t * t, each__walk_t * w)
{
    if (ti_closure_vars_vset(w->closure, t) ||
        ti_closure_do_statement(w->closure, w->query, w->e))
        return -1;
    ti_val_unsafe_drop(w->query->rval);
    w->query->rval = NULL;
    return 0;
}

static int each__walk_i(ti_item_t * item, each__walk_t * w)
{
    ti_closure_vars_item(w->closure, item);
    if (ti_closure_do_statement(w->closure, w->query, w->e))
        return -1;
    ti_val_unsafe_drop(w->query->rval);
    w->query->rval = NULL;
    return 0;
}

static int do__f_each(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const char * doc;
    const int nargs = fn_get_nargs(nd);
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

    if (ti_do_statement(query, nd->children, e) ||
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
            if (ti_thing_is_dict(thing))
            {
                each__walk_t w = {
                        .e = e,
                        .closure = closure,
                        .query = query,
                };

                if (smap_values(
                        thing->items.smap,
                        (smap_val_cb) each__walk_i,
                        &w))
                    goto fail2;
            }
            else
            {
                for (vec_each(thing->items.vec, ti_prop_t, p))
                {
                    ti_closure_vars_prop(closure, p);
                    if (ti_closure_do_statement(closure, query, e))
                        goto fail2;
                    ti_val_unsafe_drop(query->rval);
                    query->rval = NULL;
                }
            }
        }
        else
        {
            ti_name_t * name;
            ti_val_t * val;
            for (thing_t_each(thing, name, val))
            {
                ti_closure_vars_nameval(closure, (ti_val_t *) name, val);
                if (ti_closure_do_statement(closure, query, e))
                    goto fail2;
                ti_val_unsafe_drop(query->rval);
                query->rval = NULL;
            }
        }
        break;
    }
    case TI_VAL_ARR:
    {
        int64_t idx = 0;
        for (vec_each(VARR(iterval), ti_val_t, v), ++idx)
        {
            if (ti_closure_vars_val_idx(closure, v, idx))
            {
                ex_set_mem(e);
                goto fail2;
            }

            if (ti_closure_do_statement(closure, query, e))
                goto fail2;

            ti_val_unsafe_drop(query->rval);
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

        if (ti_vset_walk(
                (ti_vset_t *) iterval,
                query,
                closure,
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

    assert(query->rval == NULL);
    query->rval = (ti_val_t *) ti_nil_get();

fail2:
    ti_closure_dec(closure, query);

fail1:
    ti_val_unsafe_drop((ti_val_t *) closure);

fail0:
    ti_val_unlock(iterval, lock_was_set);
    ti_val_unsafe_drop(iterval);
    return e->nr;
}
