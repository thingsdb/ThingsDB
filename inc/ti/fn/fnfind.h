#include <ti/fn/fn.h>

typedef struct
{
    ex_t * e;
    ti_closure_t * closure;
    ti_query_t * query;
} find__walk_t;

static int find__walk_set(ti_thing_t * t, find__walk_t * w)
{
    _Bool found;
    if (ti_closure_vars_vset(w->closure, t))
        return -1;

    if (ti_closure_do_statement(w->closure, w->query, w->e))
        return -1;

    found = ti_val_as_bool(w->query->rval);
    ti_val_unsafe_drop(w->query->rval);

    if (found)
    {
        w->query->rval = (ti_val_t *) t;
        ti_incref(t);
        return 1;  /* found */
    }
    w->query->rval = NULL;
    return 0;
}

static int do__f_find(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const char * doc;
    const int nargs = fn_get_nargs(nd);
    int64_t idx = 0;
    ti_closure_t * closure;
    ti_val_t * iterval;
    int lock_was_set;

    doc = doc_find(query->rval);
    if (!doc)
        return fn_call_try("find", query, nd, e);

    if (fn_nargs_range("find", doc, 1, 2, nargs, e))
        return e->nr;

    lock_was_set = ti_val_ensure_lock(query->rval);
    iterval = query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e) ||
        fn_arg_closure("find", doc, 1, query->rval, e))
        goto fail0;

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (    ti_closure_try_wse(closure, query, e) ||
            ti_closure_inc(closure, query, e))
        goto fail1;

    switch (iterval->tp)
    {
    case TI_VAL_ARR:
        for (vec_each(VARR(iterval), ti_val_t, v), ++idx)
        {
            _Bool found;

            if (ti_closure_vars_val_idx(closure, v, idx))
            {
                ex_set_mem(e);
                goto fail2;
            }

            if (ti_closure_do_statement(closure, query, e))
                goto fail2;

            found = ti_val_as_bool(query->rval);
            ti_val_unsafe_drop(query->rval);

            if (found)
            {
                query->rval = v;
                ti_incref(v);
                goto done;
            }

            query->rval = NULL;
        }
        break;
    case TI_VAL_SET:
    {
        int rc;
        find__walk_t w = {
                .e = e,
                .closure = closure,
                .query = query,
        };

        rc = ti_vset_has_relation((ti_vset_t *) iterval)
                ? imap_walk_cp(VSET(iterval),
                        (imap_cb) find__walk_set,
                        &w,
                        (imap_destroy_cb) ti_val_unsafe_drop)
                : imap_walk(VSET(iterval), (imap_cb) find__walk_set, &w);

        if (rc > 0)
            goto done;
        if (rc < 0)
        {
            if (!e->nr)
                ex_set_mem(e);
            goto fail2;
        }
        break;
    }
    }

    assert (query->rval == NULL);
    if (nargs == 2)
    {
        /* lazy evaluation of the alternative value */
        (void) ti_do_statement(query, nd->children->next->next, e);
    }
    else
    {
        query->rval = (ti_val_t *) ti_nil_get();
    }

done:
fail2:
    ti_closure_dec(closure, query);

fail1:
    ti_val_unsafe_drop((ti_val_t *) closure);

fail0:
    ti_val_unlock(iterval, lock_was_set);
    ti_val_unsafe_drop(iterval);
    return e->nr;
}
