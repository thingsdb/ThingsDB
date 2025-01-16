/*
 * ti/forloop.c
 *
 * Note: in a for.in loop we always need to use `ti_val_unsafe_gc_drop` on
 *       props as an iteration does not have its own local stack scope.
 */
#include <ti/do.h>
#include <ti/forloop.h>
#include <ti/nil.h>
#include <ti/val.inline.h>
#include <ti/varr.h>
#include <ti/vset.h>
#include <util/imap.h>
#include <util/logger.h>

typedef struct
{
    const int nargs;
    ti_query_t * query;
    cleri_node_t * vars_nd;
    cleri_node_t * code_nd;
    ex_t * e;
} forloop__walk_t;

typedef struct
{
    ti_prop_t * prop0;
    ti_prop_t * prop1;
} forloop__props_t;

static inline void forloop__set_prop(
        int nargs,
        cleri_node_t * vars_nd,
        ti_name_t * name,
        ti_val_t * val,
        forloop__props_t * props)
{
    switch(nargs)
    {
    default:
    case 2:
        props->prop0 = vars_nd->children->next->data;
        ti_incref(val);
        ti_val_unsafe_gc_drop(props->prop0->val);
        props->prop0->val = val;
        /* fall through */
    case 1:
        props->prop1 = vars_nd->data;
        ti_incref(name);
        ti_val_unsafe_gc_drop(props->prop1->val);
        props->prop1->val = (ti_val_t *) name;
        /* fall through */
    case 0:
        break;
    }
}

static inline void forloop__unset_prop(
        int nargs,
        cleri_node_t * vars_nd,
        forloop__props_t * props)
{
    switch(nargs)
    {
    default:
    case 2:
        vars_nd->children->next->data = props->prop0;
        /* fall through */
    case 1:
        vars_nd->data = props->prop1;
        /* fall through */
    case 0:
        break;
    }
}

static int forloop__walk_thing(ti_item_t * item, forloop__walk_t * w)
{
    int rc;
    forloop__props_t props = {0};

    forloop__set_prop(
            w->nargs,
            w->vars_nd,
            (ti_name_t *) item->key,
            item->val,
            &props);

    w->query->rval = NULL;
    rc = ti_do_statement(w->query, w->code_nd, w->e);
    forloop__unset_prop(w->nargs, w->vars_nd, &props);
    switch (rc)
    {
    case EX_SUCCESS:
        ti_val_unsafe_drop(w->query->rval);
        return 0;
    case EX_CONTINUE:
        ti_val_drop(w->query->rval);  /* may be NULL */
        w->e->nr = 0;
        return 0;
    case EX_BREAK:
        ti_val_drop(w->query->rval);  /* may be NULL */
        w->e->nr = 0;
        return 1;  /* success, but stop the loop */
    }
    return -1;  /* fail (or EX_RETURN), leave query->rval as-is */
}

static int forloop__walk_set(ti_thing_t * t, forloop__walk_t * w)
{
    int rc;
    ti_prop_t * prop0 = NULL;
    ti_prop_t * prop1 = NULL;

    switch(w->nargs)
    {
    default:
    case 2:
        prop0 = w->vars_nd->children->next->data;
        ti_val_unsafe_gc_drop(prop0->val);
        prop0->val = t->id
                ? (ti_val_t *) ti_vint_create((int64_t) t->id)
                : (ti_val_t *) ti_nil_get();

        if (!prop0->val)
            return ex_set_mem(w->e), w->e->nr;
        /* fall through */
    case 1:
        prop1 = w->vars_nd->data;
        ti_incref(t);
        ti_val_unsafe_gc_drop(prop1->val);
        prop1->val = (ti_val_t *) t;
        /* fall through */
    case 0:
        break;
    }

    w->query->rval = NULL;

    rc = ti_do_statement(w->query, w->code_nd, w->e);

    switch(w->nargs)
    {
    default:
    case 2:
        w->vars_nd->children->next->data = prop0;
        /* fall through */
    case 1:
        w->vars_nd->data = prop1;
        /* fall through */
    case 0:
        break;
    }

    switch (rc)
    {
    case EX_SUCCESS:
        ti_val_unsafe_drop(w->query->rval);
        return 0;
    case EX_CONTINUE:
        ti_val_drop(w->query->rval);  /* may be NULL */
        w->e->nr = 0;
        return 0;
    case EX_BREAK:
        ti_val_drop(w->query->rval);  /* may be NULL */
        w->e->nr = 0;
        return 1;  /* success, but stop the loop */
    }
    return -1;  /* fail (or EX_RETURN), leave query->rval as-is */
}

int ti_forloop_no_iter(
        ti_query_t * query,
        cleri_node_t * UNUSED(vars_nd),
        cleri_node_t * UNUSED(code_nd),
        ex_t * e)
{
    ex_set(e, EX_TYPE_ERROR,
            "type `%s` is not iterable", ti_val_str(query->rval));
    return e->nr;
}

int ti_forloop_arr(
        ti_query_t * query,
        cleri_node_t * vars_nd,
        cleri_node_t * code_nd,
        ex_t * e)
{
    int nargs = 0;
    int lock_was_set;
    ti_varr_t * varr = (ti_varr_t *) query->rval;
    int64_t idx = 0;

    if (!varr->vec->n)
        return 0;

    nargs = ti_do_prepare_for_loop(query, vars_nd);
    if (nargs < 0)
        return ex_set_mem(e), e->nr;

    lock_was_set = ti_val_ensure_lock(query->rval);

    for (vec_each(varr->vec, ti_val_t, v), ++idx)
    {
        ti_prop_t * prop;
        switch(nargs)
        {
        default:
        case 2:
            prop = vars_nd->children->next->data;
            ti_val_unsafe_gc_drop(prop->val);
            prop->val = (ti_val_t *) ti_vint_create(idx);
            if (!prop->val)
            {
                ex_set_mem(e);
                goto fail0;
            }
            /* fall through */
        case 1:
            prop = vars_nd->data;
            ti_incref(v);
            ti_val_unsafe_gc_drop(prop->val);
            prop->val = v;
            /* fall through */
        case 0:
            break;
        }

        query->rval = NULL;
        switch (ti_do_statement(query, code_nd, e))
        {
        case EX_SUCCESS:
            ti_val_unsafe_drop(query->rval);
            continue;
        case EX_CONTINUE:
            ti_val_drop(query->rval);  /* may be NULL */
            e->nr = 0;
            continue;
        case EX_BREAK:
            ti_val_drop(query->rval);  /* may be NULL */
            e->nr = 0;
            goto done;  /* success, but stop the loop */
        }
        goto fail0;  /* EX_RETURN must leave the value alone */
    }

done:
    query->rval = (ti_val_t *) ti_nil_get();

fail0:
    ti_val_unlock((ti_val_t *) varr, lock_was_set);
    ti_val_unsafe_drop((ti_val_t *) varr);
    return e->nr;
}

int ti_forloop_set(
        ti_query_t * query,
        cleri_node_t * vars_nd,
        cleri_node_t * code_nd,
        ex_t * e)
{
    int rc;
    int nargs = 0;
    int lock_was_set;
    ti_vset_t * vset = (ti_vset_t *) query->rval;

    if (!vset->imap->n)
        return 0;

    nargs = ti_do_prepare_for_loop(query, vars_nd);
    if (nargs < 0)
        return ex_set_mem(e), e->nr;

    lock_was_set = ti_val_ensure_lock(query->rval);

    forloop__walk_t w = {
            .nargs = nargs,
            .query = query,
            .vars_nd = vars_nd,
            .code_nd = code_nd,
            .e = e,
    };

    query->rval = NULL;

    rc = (query->change && ti_vset_has_relation(vset))
            ? imap_walk_cp(vset->imap,
                    (imap_cb) forloop__walk_set,
                    &w,
                    (imap_destroy_cb) ti_val_unsafe_drop)
            : imap_walk(vset->imap, (imap_cb) forloop__walk_set, &w);

    if (rc >= 0)
        query->rval = (ti_val_t *) ti_nil_get();
    else if (!e->nr)
        ex_set_mem(e);

    ti_val_unlock((ti_val_t *) vset, lock_was_set);
    ti_val_unsafe_drop((ti_val_t *) vset);
    return e->nr;
}

int ti_forloop_thing(
        ti_query_t * query,
        cleri_node_t * vars_nd,
        cleri_node_t * code_nd,
        ex_t * e)
{
    int nargs = 0;
    int lock_was_set;
    forloop__props_t props = {0};
    ti_thing_t * thing = (ti_thing_t *) query->rval;

    if (!ti_thing_n(thing))
        return 0;

    nargs = ti_do_prepare_for_loop(query, vars_nd);
    if (nargs < 0)
        return ex_set_mem(e), e->nr;

    lock_was_set = ti_val_ensure_lock(query->rval);

    if (!ti_thing_is_object(thing))
    {
        ti_name_t * name;
        ti_val_t * val;
        for (thing_t_each(thing, name, val))
        {
            int rc;
            forloop__set_prop(nargs, vars_nd, name, val, &props);

            query->rval = NULL;
            rc = ti_do_statement(query, code_nd, e);
            forloop__unset_prop(nargs, vars_nd, &props);
            switch (rc)
            {
            case EX_SUCCESS:
                ti_val_unsafe_drop(query->rval);
                continue;
            case EX_CONTINUE:
                ti_val_drop(query->rval);  /* may be NULL */
                e->nr = 0;
                continue;
            case EX_BREAK:
                ti_val_drop(query->rval);  /* may be NULL */
                e->nr = 0;
                goto done;  /* success, but stop the loop */
            }
            goto fail0;  /* EX_RETURN must leave the value alone */
        }
        goto done;
    }

    if (ti_thing_is_dict(thing))
    {
        forloop__walk_t w = {
                .nargs = nargs,
                .query = query,
                .vars_nd = vars_nd,
                .code_nd = code_nd,
                .e = e,
        };

        if (smap_values(
                thing->items.smap,
                (smap_val_cb) forloop__walk_thing,
                &w) >= 0)
            goto done;
        goto fail0;
    }

    for (vec_each(thing->items.vec, ti_prop_t, p))
    {
        int rc;
        forloop__set_prop(nargs, vars_nd, p->name, p->val, &props);

        query->rval = NULL;
        rc = ti_do_statement(query, code_nd, e);
        forloop__unset_prop(nargs, vars_nd, &props);
        switch (rc)
        {
        case EX_SUCCESS:
            ti_val_unsafe_drop(query->rval);
            continue;
        case EX_CONTINUE:
            ti_val_drop(query->rval);  /* may be NULL */
            e->nr = 0;
            continue;
        case EX_BREAK:
            ti_val_drop(query->rval);  /* may be NULL */
            e->nr = 0;
            goto done;  /* success, but stop the loop */
        }
        goto fail0;  /* EX_RETURN must leave the value alone */
    }

done:
    query->rval = (ti_val_t *) ti_nil_get();

fail0:
    ti_val_unlock((ti_val_t *) thing, lock_was_set);
    ti_val_unsafe_drop((ti_val_t *) thing);
    return e->nr;
}
