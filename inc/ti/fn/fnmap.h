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
        ti_varr_append(w->varr, (void **) &w->query->rval, w->e))
        return -1;
    w->query->rval = NULL;
    return 0;
}

static int do__f_map(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const char * doc;
    const int nargs = langdef_nd_n_function_params(nd);
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

    if (ti_do_statement(query, nd->children->node, e) ||
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
            for (vec_each(thing->items.vec, ti_prop_t, p))
            {
                if (ti_closure_vars_prop(closure, p, e) ||
                    ti_closure_do_statement(closure, query, e) ||
                    ti_varr_append(retvarr, (void **) &query->rval, e))
                    goto fail2;
                query->rval = NULL;
            }
        }
        else
        {
            ti_name_t * name;
            ti_val_t * val;
            for (thing_t_each(thing, name, val))
            {
                if (ti_closure_vars_nameval(closure, (ti_val_t *) name, val, e) ||
                    ti_closure_do_statement(closure, query, e) ||
                    ti_varr_append(retvarr, (void **) &query->rval, e))
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
                ti_varr_append(retvarr, (void **) &query->rval, e))
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
        if (imap_walk(VSET(iterval), (imap_cb) map__walk_set, &w))
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
