#include <ti/fn/fn.h>

typedef struct
{
    ex_t * e;
    ti_closure_t * closure;
    ti_query_t * query;
    ti_vset_t * vset;
} filter__walk_set_t;

static int filter__walk_set(ti_thing_t * t, filter__walk_set_t * w)
{
    if (ti_closure_vars_vset(w->closure, t) ||
        ti_closure_do_statement(w->closure, w->query, w->e))
        return -1;

    if (ti_val_as_bool(w->query->rval))
    {
        if (ti_vset_add(w->vset, t))
            return -1;
        ti_incref(t);
    }

    ti_val_unsafe_drop(w->query->rval);
    w->query->rval = NULL;
    return 0;
}

typedef struct
{
    ex_t * e;
    ti_closure_t * closure;
    ti_query_t * query;
    ti_thing_t * thing;
} filter__walk_i_t;

static int filter__walk_i(ti_item_t * item, filter__walk_i_t * w)
{
    if (ti_closure_vars_item(w->closure, item, w->e) ||
        ti_closure_do_statement(w->closure, w->query, w->e))
        return -1;

    if (ti_val_as_bool(w->query->rval))
    {
        if (ti_val_make_variable(&item->val, w->e) ||
            !ti_thing_i_item_add(w->thing, item->key, item->val))
            return -1;
        ti_incref(item->key);
        ti_incref(item->val);
    }

    ti_val_unsafe_drop(w->query->rval);
    w->query->rval = NULL;
    return 0;
}

static int do__f_filter(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const char * doc;
    const int nargs = langdef_nd_n_function_params(nd);
    ti_val_t * retval = NULL;
    ti_closure_t * closure;
    ti_val_t * iterval;
    int lock_was_set;

    doc = doc_filter(query->rval);
    if (!doc)
        return fn_call_try("filter", query, nd, e);

    if (fn_nargs("filter", doc, 1, nargs, e))
        return e->nr;

    lock_was_set = ti_val_ensure_lock(query->rval);
    iterval = query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e) ||
        fn_arg_closure("filter", doc, 1, query->rval, e))
        goto fail0;

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (    ti_closure_try_wse(closure, query, e) ||
            ti_closure_inc(closure, query, e))
        goto fail1;

    switch (iterval->tp)
    {
    case TI_VAL_THING:
    {
        ti_thing_t * thing, * t = (ti_thing_t *) iterval;

        if (ti_thing_is_dict(t))
        {
            thing = ti_thing_i_create(0, query->collection);
            if (!thing)
                goto fail2;

            retval = (ti_val_t *) thing;

            filter__walk_i_t w = {
                    .closure = closure,
                    .e = e,
                    .query = query,
                    .thing = thing,
            };

            if (smap_values(t->items.smap, (smap_val_cb) filter__walk_i, &w))
                goto fail2;
            break;
        }

        thing = ti_thing_o_create(0, ti_thing_n(t), query->collection);
        if (!thing)
            goto fail2;

        retval = (ti_val_t *) thing;

        if (ti_thing_is_object(t))
        {
            for (vec_each(t->items.vec, ti_prop_t, p))
            {
                if (ti_closure_vars_prop(closure, p, e) ||
                    ti_closure_do_statement(closure, query, e))
                    goto fail2;

                if (ti_val_as_bool(query->rval))
                {
                    if (    ti_val_make_variable(&p->val, e) ||
                            !ti_thing_p_prop_add(thing, p->name, p->val))
                        goto fail2;
                    ti_incref(p->name);
                    ti_incref(p->val);
                }

                ti_val_unsafe_drop(query->rval);
                query->rval = NULL;
            }
        }
        else
        {
            ti_name_t * name;
            ti_val_t * val;
            for (thing_t_each(t, name, val))
            {
                if (ti_closure_vars_nameval(closure, (ti_val_t *) name, val, e) ||
                    ti_closure_do_statement(closure, query, e))
                    goto fail2;

                if (ti_val_as_bool(query->rval))
                {
                    if (    ti_val_make_variable(&val, e) ||
                            !ti_thing_p_prop_add(thing, name, val))
                        goto fail2;
                    ti_incref(name);
                    ti_incref(val);
                }

                ti_val_unsafe_drop(query->rval);
                query->rval = NULL;
            }

        }
        break;
    }
    case TI_VAL_ARR:
    {
        int64_t idx = 0;
        ti_varr_t * varr = ti_varr_create(VARR(iterval)->n);
        if (!varr)
            goto fail2;

        retval = (ti_val_t *) varr;

        for (vec_each(VARR(iterval), ti_val_t, v), ++idx)
        {
            if (ti_closure_vars_val_idx(closure, v, idx) ||
                ti_closure_do_statement(closure, query, e))
                goto fail2;

            if (ti_val_as_bool(query->rval))
            {
                ti_incref(v);
                VEC_push(varr->vec, v);

                if (ti_val_is_thing(v))
                    varr->flags |= TI_VARR_FLAG_MHT;
            }

            ti_val_unsafe_drop(query->rval);
            query->rval = NULL;

        }
        (void) vec_shrink(&varr->vec);
        break;
    }
    case TI_VAL_SET:
    {
        filter__walk_set_t w = {
                .e = e,
                .closure = closure,
                .query = query,
                .vset = ti_vset_create(),
        };

        if (!w.vset)
            goto fail2;

        retval = (ti_val_t *) w.vset;

        if (imap_walk(VSET(iterval), (imap_cb) filter__walk_set, &w))
            goto fail2;
    }
    }

    assert (query->rval == NULL);
    query->rval = retval;

    goto done;

fail2:
    if (!e->nr)
        ex_set_mem(e);
    ti_val_drop(retval);

done:
    ti_closure_dec(closure, query);

fail1:
    ti_val_unsafe_drop((ti_val_t *) closure);

fail0:
    ti_val_unlock(iterval, lock_was_set);
    ti_val_unsafe_drop(iterval);
    return e->nr;
}
