#include <ti/fn/fn.h>

typedef struct
{
    ex_t * e;
    ti_closure_t * closure;
    ti_query_t * query;
} reduce__walk_t;

static int reduce__walk_set(ti_thing_t * t, reduce__walk_t * w)
{
    ti_prop_t * prop;

    if (!w->query->rval)
    {
        ti_incref(t);
        w->query->rval = (ti_val_t *) t;
        return 0;
    }

    switch(w->closure->vars->n)
    {
    default:
    case 3:
        prop = VEC_get(w->closure->vars, 2);
        ti_val_unsafe_drop(prop->val);
        prop->val = t->id
                ? (ti_val_t *) ti_vint_create((int64_t) t->id)
                : (ti_val_t *) ti_nil_get();
        if (!prop->val)
        {
            ex_set_mem(w->e);
            return w->e->nr;
        }
        /* fall through */
    case 2:
        prop = VEC_get(w->closure->vars, 1);
        ti_incref(t);
        ti_val_unsafe_drop(prop->val);
        prop->val = (ti_val_t *) t;
        /* fall through */
    case 1:
        prop = VEC_get(w->closure->vars, 0);
        ti_val_unsafe_drop(prop->val);
        prop->val = w->query->rval;
        w->query->rval = NULL;
        break;
    case 0:
        ti_val_unsafe_drop(w->query->rval);
        w->query->rval = NULL;
        break;
    }

    return ti_closure_do_statement(w->closure, w->query, w->e);
}

static int do__f_reduce(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const char * doc;
    const int nargs = fn_get_nargs(nd);
    ti_closure_t * closure;
    ti_val_t * lockval;
    int lock_was_set;

    doc = doc_reduce(query->rval);
    if (!doc)
        return fn_call_try("reduce", query, nd, e);

    if (fn_nargs_range("reduce", doc, 1, 2, nargs, e))
        return e->nr;

    lock_was_set = ti_val_ensure_lock(query->rval);
    lockval = query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e) ||
        fn_arg_closure("reduce", doc, 1, query->rval, e))
        goto fail0;

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (    ti_closure_try_wse(closure, query, e) ||
            ti_closure_inc(closure, query, e))
        goto fail1;

    switch (lockval->tp)
    {
    case TI_VAL_ARR:
    {
        vec_t * vec = VARR(lockval);

        if (nargs == 2)
        {
            if (ti_do_statement(query, nd->children->next->next, e))
                goto fail2;
        }
        else if (vec->n == 0)
        {
            ex_set(e, EX_LOOKUP_ERROR,
                    "reduce on empty list with no initial value set");
            goto fail2;
        }
        else
        {
            query->rval = VEC_first(vec);
            ti_incref(query->rval);
        }

        for (size_t idx = nargs & 1, m = vec->n; idx < m; ++idx)
        {
            ti_val_t * v;
            ti_prop_t * prop;
            switch(closure->vars->n)
            {
            default:
            case 3:
                prop = VEC_get(closure->vars, 2);
                ti_val_unsafe_drop(prop->val);
                prop->val = (ti_val_t *) ti_vint_create(idx);
                if (!prop->val)
                {
                    ex_set_mem(e);
                    goto fail2;
                }
                /* fall through */
            case 2:
                prop = VEC_get(closure->vars, 1);
                v = VEC_get(vec, idx);
                ti_incref(v);
                ti_val_unsafe_drop(prop->val);
                prop->val = v;
                /* fall through */
            case 1:
                prop = VEC_get(closure->vars, 0);
                ti_val_unsafe_drop(prop->val);
                prop->val = query->rval;
                query->rval = NULL;
                break;
            case 0:
                ti_val_unsafe_drop(query->rval);
                query->rval = NULL;
                break;
            }
            if (ti_closure_do_statement(closure, query, e))
                goto fail2;
        }
        break;
    }
    case TI_VAL_SET:
    {

        if (nargs == 2)
        {
            if (ti_do_statement(query, nd->children->next->next, e))
                goto fail2;
        }
        else if (VSET(lockval)->n == 0)
        {
            ex_set(e, EX_LOOKUP_ERROR,
                    "reduce on empty set with no initial value set");
            goto fail2;
        }

        reduce__walk_t w = {
                .e = e,
                .closure = closure,
                .query = query,
        };
        if (ti_vset_walk(
                (ti_vset_t *) lockval,
                query,
                closure,
                (imap_cb) reduce__walk_set,
                &w) && !e->nr)
            ex_set_mem(e);
    }
    }

fail2:
    ti_closure_dec(closure, query);

fail1:
    ti_val_unsafe_drop((ti_val_t *) closure);

fail0:
    ti_val_unlock(lockval, lock_was_set);
    ti_val_unsafe_drop(lockval);
    return e->nr;
}
