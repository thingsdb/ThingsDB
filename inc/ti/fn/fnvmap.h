#include <ti/fn/fn.h>

typedef struct
{
    ex_t * e;
    ti_closure_t * closure;
    ti_query_t * query;
    ti_thing_t * thing;
} vmap__walk_i_t;

static int vmap__walk_i(ti_item_t * item, vmap__walk_i_t * w)
{
    if (ti_closure_vars_val(w->closure, item->val, w->e) ||
        ti_closure_do_statement(w->closure, w->query, w->e) ||
        ti_val_make_variable(&w->query->rval, w->e) ||
        !ti_thing_i_item_add(w->thing, item->key, w->query->rval))
        return -1;

    ti_incref(item->key);
    w->query->rval = NULL;
    return 0;
}

static int do__f_vmap(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_thing_t * othing, * nthing;
    ti_closure_t * closure;
    int lock_was_set;

    if (!ti_val_is_thing(query->rval))
        return fn_call_try("vmap", query, nd, e);

    if (fn_nargs("vmap", DOC_THING_VMAP, 1, nargs, e))
        return e->nr;

    lock_was_set = ti_val_ensure_lock(query->rval);
    othing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e) ||
        fn_arg_closure("vmap", DOC_THING_VMAP, 1, query->rval, e))
        goto fail0;

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (    ti_closure_try_wse(closure, query, e) ||
            ti_closure_inc(closure, query, e))
        goto fail1;


    if (ti_thing_is_dict(othing))
    {
        vmap__walk_i_t w = {
                .closure = closure,
                .e = e,
                .query = query,
                .thing = nthing,
        };

        nthing = ti_thing_i_create(0, query->collection);

        if (!nthing || smap_values(
                othing->items.smap,
                (smap_val_cb) vmap__walk_i,
                &w))
            goto fail2;
    }
    else
    {
        nthing = ti_thing_o_create(0, ti_thing_n(othing), query->collection);
        if (!nthing)
            goto fail2;

        if (ti_thing_is_object(othing))
        {
            /* the original thing is an object */
            for (vec_each(othing->items.vec, ti_prop_t, prop))
            {
                if (ti_closure_vars_val(closure, prop->val, e) ||
                    ti_closure_do_statement(closure, query, e) ||
                    ti_val_make_variable(&query->rval, e) ||
                    !ti_thing_p_prop_add(nthing, prop->name, query->rval))
                    goto fail2;

                ti_incref(prop->name);
                query->rval = NULL;
            }
        }
        else
        {
            /* the original thing is an instance */
            ti_name_t * name;
            ti_val_t * val;
            for (thing_t_each(othing, name, val))
            {
                if (ti_closure_vars_val(closure, val, e) ||
                    ti_closure_do_statement(closure, query, e) ||
                    ti_val_make_variable(&query->rval, e) ||
                    !ti_thing_p_prop_add(nthing, name, query->rval))
                    goto fail2;

                ti_incref(name);
                query->rval = NULL;
            }
        }
    }

    query->rval = (ti_val_t *) nthing;
    goto done;

fail2:
    if (!e->nr)
        ex_set_mem(e);
    ti_val_drop((ti_val_t *) nthing);

done:
    ti_closure_dec(closure, query);

fail1:
    ti_val_unsafe_drop((ti_val_t *) closure);

fail0:
    ti_val_unlock((ti_val_t *) othing, lock_was_set);
    ti_val_unsafe_drop((ti_val_t *) othing);
    return e->nr;
}
