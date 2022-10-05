#include <ti/fn/fn.h>

typedef struct
{
    ex_t * e;
    ti_closure_t * closure;
    ti_query_t * query;
    ti_varr_t * varr;
} map__walk_t;

static int map__walk_set(ti_thing_t * t, map__walk_t * w)
{
    if (ti_closure_vars_vset(w->closure, t) ||
        ti_closure_do_statement(w->closure, w->query, w->e) ||
        ti_val_varr_append(w->varr, &w->query->rval, w->e))
        return -1;
    w->query->rval = NULL;
    return 0;
}

static int map__walk_i(ti_item_t * item, map__walk_t * w)
{
    ti_closure_vars_item(w->closure, item);
    if (ti_closure_do_statement(w->closure, w->query, w->e) ||
        ti_val_varr_append(w->varr, &w->query->rval, w->e))
        return -1;
    w->query->rval = NULL;
    return 0;
}

static int do__f_map(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const char * doc;
    const int nargs = fn_get_nargs(nd);
    size_t n;
    ti_varr_t * retvarr;
    ti_closure_t * closure;
    ti_val_t * iterval;
    int lock_was_set;

    doc = doc_map(query->rval);
    if (!doc)
        return fn_call_try("map", query, nd, e);

    if (fn_nargs("map", doc, 1, nargs, e))
        return e->nr;

    lock_was_set = ti_val_ensure_lock(query->rval);
    iterval = query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e) ||
        fn_arg_closure("map", doc, 1, query->rval, e))
        goto fail0;

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    n = ti_val_get_len(iterval);

    if (ti_closure_try_wse(closure, query, e) ||
        ti_closure_inc(closure, query, e))
        goto fail1;

    retvarr = ti_varr_create(n);
    if (!retvarr)
        goto fail2;

    switch (iterval->tp)
    {
    case TI_VAL_THING:
    {
        ti_thing_t * thing = (ti_thing_t *) iterval;

        if (ti_thing_is_object(thing))
        {
            if (ti_thing_is_dict(thing))
            {
                map__walk_t w = {
                        .e = e,
                        .closure = closure,
                        .query = query,
                        .varr = retvarr,
                };

                if (smap_values(
                        thing->items.smap,
                        (smap_val_cb) map__walk_i,
                        &w))
                    goto fail2;
            }
            else
            {
                for (vec_each(thing->items.vec, ti_prop_t, p))
                {
                    ti_closure_vars_prop(closure, p);
                    if (ti_closure_do_statement(closure, query, e) ||
                        ti_val_varr_append(retvarr, &query->rval, e))
                        goto fail2;
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
                if (ti_closure_do_statement(closure, query, e) ||
                    ti_val_varr_append(retvarr, &query->rval, e))
                    goto fail2;
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
            if (ti_closure_vars_val_idx(closure, v, idx) ||
                ti_closure_do_statement(closure, query, e) ||
                ti_val_varr_append(retvarr, &query->rval, e))
                goto fail2;
            query->rval = NULL;
        }
        break;
    }
    case TI_VAL_SET:
    {
        map__walk_t w = {
                .e = e,
                .closure = closure,
                .query = query,
                .varr = retvarr,
        };
        if (ti_vset_walk(
                (ti_vset_t *) iterval,
                query,
                closure,
                (imap_cb) map__walk_set,
                &w))
            goto fail2;
        break;
    }
    }

    assert (query->rval == NULL);
    assert (retvarr->vec->n == n);

    query->rval = (ti_val_t *) retvarr;
    goto done;

fail2:
    if (!e->nr)
        ex_set_mem(e);
    ti_val_drop((ti_val_t *) retvarr);

done:
    ti_closure_dec(closure, query);

fail1:
    ti_val_unsafe_drop((ti_val_t *) closure);

fail0:
    ti_val_unlock(iterval, lock_was_set);
    ti_val_unsafe_drop(iterval);
    return e->nr;
}
