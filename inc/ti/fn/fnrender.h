#include <ti/fn/fn.h>


static int render__scoped(ti_query_t *query, cleri_node_t *nd, ex_t *e)
{
    vec_t *vec;
    ti_thing_t *thing;
    ti_template_t *tpl = (ti_template_t *) query->rval;

    if (ti_template_try_lock(tpl, e))
        return e->nr;

    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e) ||
        fn_arg_thing("render", DOC_TEMPLATE_RENDER, 1, query->rval, e))
        goto fail0;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_thing_is_dict(thing))
    {
        ti_raw_t *incompatible;
        if (ti_thing_i_to_p(thing, &incompatible))
        {
            if (incompatible)
                ex_set(e, EX_VALUE_ERROR,
                        "cannot set property `%s` as scope variable: "
                        "invalid variable name",
                        ti_raw_as_printable_str(incompatible));
            else
                ex_set_mem(e);
            goto fail1;
        }
    }

    vec = vec_new(ti_thing_n(thing));
    if (!vec)
    {
        ex_set_mem(e);
        goto fail1;
    }

    if (ti_thing_is_object(thing))
    {
        for (vec_each(thing->items.vec, ti_prop_t, prop))
        {
            ti_prop_t *p = ti_prop_dup(prop);
            if (!p)
            {
                ex_set_mem(e);
                goto fail2;
            }
            VEC_push(vec, p);
        }
    }
    else
    {
        ti_name_t *name;
        ti_val_t *val;
        for (thing_t_each(thing, name, val))
        {
            ti_prop_t * p = ti_prop_create(name, val);
            if (!p)
            {
                ex_set_mem(e);
                goto fail2;
            }
            ti_incref(name);
            ti_incref(val);
            VEC_push(vec, p);
        }
    }
    if (vec_extend(&query->vars, vec->data, vec->n))
        goto fail2;

    (void) ti_template_compile(tpl, query, e);
    query->vars->n -= vec->n;
fail2:
    vec_destroy(vec, (vec_destroy_cb) ti_prop_destroy);
fail1:
    ti_val_unsafe_gc_drop((ti_val_t *) thing);
fail0:
    ti_template_unlock(tpl);
    ti_val_unsafe_drop((ti_val_t *) tpl);
    return e->nr;
}

static int do__f_render(ti_query_t *query, cleri_node_t *nd, ex_t *e)
{
    const int nargs = fn_get_nargs(nd);
    ti_template_t *tpl = (ti_template_t *) query->rval;

    if (!ti_val_is_template(query->rval))
        return fn_call_try("render", query, nd, e);

    if (fn_nargs_max("render", DOC_TEMPLATE_RENDER, 1, nargs, e))
        return e->nr;

    if (!query->change && (tpl->flags & TI_TEMPLATE_FLAG_WSE))
    {
        ex_set(e, EX_OPERATION,
            "template with side effects require a change but none is created; "
            "use `wse(...)` or append `!` to enforce a change"DOC_WSE);
        return e->nr;
    }

    if (nargs == 1)
        return render__scoped(query, nd, e);

    if (ti_template_try_lock(tpl, e))
        return e->nr;

    query->rval = NULL;
    (void) ti_template_compile(tpl, query, e);

    ti_template_unlock(tpl);
    ti_val_unsafe_drop((ti_val_t *) tpl);
    return e->nr;
}
