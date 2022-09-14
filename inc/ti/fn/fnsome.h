#include <ti/fn/fn.h>

typedef struct
{
    ex_t * e;
    ti_closure_t * closure;
    ti_query_t * query;
} some__walk_t;

static int some__walk_set(ti_thing_t * t, some__walk_t * w)
{
    _Bool some;
    if (ti_closure_vars_vset(w->closure, t) ||
        ti_closure_do_statement(w->closure, w->query, w->e))
        return -1;

    some = ti_val_as_bool(w->query->rval);
    ti_val_unsafe_drop(w->query->rval);

    if (some)
        return 1;  /* walking stops here */

    w->query->rval = NULL;
    return 0;
}

static int do__f_some(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const char * doc;
    const int nargs = fn_get_nargs(nd);
    _Bool some = false;
    int lock_was_set;
    ti_val_t * iterval;
    ti_closure_t * closure;

    doc = doc_some(query->rval);
    if (!doc)
        return fn_call_try("some", query, nd, e);

    if (fn_nargs("some", doc, 1, nargs, e))
        return e->nr;

    lock_was_set = ti_val_ensure_lock(query->rval);
    iterval = query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e) ||
        fn_arg_closure("some", doc, 1, query->rval, e))
        goto fail0;

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (    ti_closure_try_wse(closure, query, e) ||
            ti_closure_inc(closure, query, e))
        goto fail1;

    switch (iterval->tp)
    {
    case TI_VAL_ARR:
    {
        int64_t idx  = 0;
        vec_t * vec = ((ti_varr_t *) iterval)->vec;

        for (vec_each(vec, ti_val_t, v), ++idx)
        {
            if (ti_closure_vars_val_idx(closure, v, idx))
            {
                ex_set_mem(e);
                goto fail2;
            }

            if (ti_closure_do_statement(closure, query, e))
                goto fail2;

            some = ti_val_as_bool(query->rval);
            ti_val_unsafe_drop(query->rval);

            if (some)
                break;

            query->rval = NULL;
        }
        break;
    }
    case TI_VAL_SET:
    {
        some__walk_t w = {
                .e = e,
                .closure = closure,
                .query = query,
        };
        int rc = ti_vset_walk(
                (ti_vset_t *) iterval,
                query,
                closure,
                (imap_cb) some__walk_set,
                &w);
        switch (rc)
        {
        case -1:
            if (!e->nr)
                ex_set_mem(e);
            goto fail2;
        case 1:
            some = true;
        }
        break;
    }
    }

    query->rval = (ti_val_t *) ti_vbool_get(some);
fail2:
    ti_closure_dec(closure, query);
fail1:
    ti_val_unsafe_drop((ti_val_t *) closure);
fail0:
    ti_val_unlock(iterval, lock_was_set);
    ti_val_unsafe_drop(iterval);
    return e->nr;
}
