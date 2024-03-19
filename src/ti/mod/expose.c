/*
 * ti/mod/expose.c
 */
#include <ti/fn/fn.h>
#include <ti/mod/expose.h>
#include <ti/val.inline.h>
#include <ti/vp.t.h>
#include <tiinc.h>
#include <util/logger.h>

ti_mod_expose_t * ti_mod_expose_create(void)
{
    ti_mod_expose_t * expose = calloc(1, sizeof(ti_mod_expose_t));
    return expose;
}

static void expose__item_destroy(ti_item_t * item)
{
    ti_val_drop((ti_val_t *) item->key);
    ti_val_drop(item->val);
    free(item);
}

void ti_mod_expose_destroy(ti_mod_expose_t * expose)
{
    if (!expose)
        return;
    free(expose->deep);
    free(expose->load);
    free(expose->doc);
    ti_val_drop((ti_val_t *) expose->def);
    ti_val_drop((ti_val_t *) expose->closure);
    vec_destroy(expose->argmap, (vec_destroy_cb) expose__item_destroy);
    vec_destroy(expose->defaults, (vec_destroy_cb) expose__item_destroy);
    free(expose);
}

int ti_mod_closure_call(
        ti_mod_expose_t * expose,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    cleri_node_t * child = nd->children;        /* first in argument list */
    ti_closure_t * closure = expose->closure;
    vec_t * args = NULL;
    uint32_t n = closure->vars->n;
    ti_module_t * module = (ti_module_t *) query->rval;

    ti_incref(expose->closure);

    query->rval = NULL;

    if (n)
    {
        args = vec_new(n--);
        if (!args)
        {
            ex_set_mem(e);
            goto fail0;
        }

        VEC_push(args, module);
        ti_incref(module);

        while (child && n)
        {
            --n;  // outside `while` so we do not go below zero

            if (ti_do_statement(query, child, e))
                goto fail1;

            VEC_push(args, query->rval);
            query->rval = NULL;

            if (!child->next)
                break;

            child = child->next->next;
        }

        while (n--)
            VEC_push(args, ti_nil_get());
    }

    (void) ti_closure_call(closure, query, args, e);

fail1:
    vec_destroy(args, (vec_destroy_cb) ti_val_unsafe_drop);

fail0:
    ti_val_unsafe_drop((ti_val_t *) closure);
    ti_val_unsafe_drop((ti_val_t *) module);

    return e->nr;
}

int ti_mod_expose_call(
        ti_mod_expose_t * expose,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    unsigned int nargs = (unsigned int) fn_get_nargs(nd);
    ti_module_t * module = (ti_module_t *) query->rval;
    cleri_node_t * child = nd->children;
    size_t defaults_n = expose->defaults ? expose->defaults->n : 0;
    ti_future_t * future;
    ti_thing_t * thing;
    ti_name_t * deep_name = (ti_name_t *) ti_val_borrow_deep_name();
    ti_name_t * load_name = (ti_name_t *) ti_val_borrow_load_name();
    _Bool load = expose->load ? *expose->load : TI_MODULE_DEFAULT_LOAD;
    uint8_t deep = expose->deep ? *expose->deep : TI_MODULE_DEFAULT_DEEP;

    if (ti.futures_count >= TI_MAX_FUTURE_COUNT)
    {
        ex_set(e, EX_MAX_QUOTA,
                "maximum number of active futures (%u) is reached",
                TI_MAX_FUTURE_COUNT);
        return e->nr;
    }

    future = ti_future_create(query, module, 0, 1, false);
    if (!future)
    {
        ex_set_mem(e);
        return e->nr;
    }

    ti_decref(module);
    query->rval = NULL;

    /*
     * We do not crash on argmap is NULL, however, the argmap is always set
     * unless some allocation error has occurred.
     */
    if (expose->argmap)
    {
        size_t sz = expose->argmap->n + defaults_n;
        thing = ti_thing_o_create(0, sz, NULL);
        if (!thing)
        {
            ex_set_mem(e);
            goto fail0;
        }

        for (vec_each(expose->argmap, ti_item_t, item))
        {
            if (item->key->n == 1 && *item->key->data == '*')
            {
                if (child)
                {
                    ti_thing_t * tsrc;
                    ti_val_t * deep_val, * load_val;

                    if (ti_do_statement(query, child, e))
                        goto fail1;

                    switch(query->rval->tp)
                    {
                    case TI_VAL_THING:
                        tsrc = (ti_thing_t *) query->rval;

                        deep_val = ti_thing_val_weak_by_name(tsrc, deep_name);
                        load_val = ti_thing_val_weak_by_name(tsrc, load_name);

                        if (deep_val && ti_deep_from_val(deep_val, &deep, e))
                            goto fail1;

                        if (load_val)
                            load = ti_val_as_bool(load_val);

                        if (ti_thing_assign(thing, tsrc, NULL, e))
                            goto fail1;
                        break;
                    case TI_VAL_NIL:
                        break;
                    default:
                        ex_set(e, EX_TYPE_ERROR,
                            "expecting argument `*` to be of "
                            "type `"TI_VAL_THING_S"` or `"TI_VAL_NIL_S"` "
                            "but got type `%s` instead",
                            ti_val_str(query->rval));
                        goto fail1;
                    }

                    child = child->next ? child->next->next : NULL;

                    ti_val_unsafe_drop(query->rval);
                    query->rval = NULL;
                }
                continue;
            }

            if (child)
            {
                if (ti_do_statement(query, child, e))
                    goto fail1;

                child = child->next ? child->next->next : NULL;

                if (deep_name == (ti_name_t *) item->key)
                {
                    if (ti_deep_from_val(query->rval, &deep, e))
                        goto fail1;
                    continue;
                }

                if (load_name == (ti_name_t *) item->key)
                {
                    load = ti_val_as_bool(query->rval);
                    continue;
                }
            }
            else
            {
                if (deep_name == (ti_name_t *) item->key ||
                    load_name == (ti_name_t *) item->key)
                    continue;

                query->rval = item->val;
                ti_incref(query->rval);
            }

            if (ti_thing_o_add(thing, item->key, query->rval))
            {
                ex_set_mem(e);
                goto fail1;
            }

            ti_incref(item->key);
            query->rval = NULL;
        }
        nargs = nargs > expose->argmap->n ? (nargs-expose->argmap->n+1) : 1;
    }
    else if (nargs)
    {
        ti_val_t * deep_val, * load_val;

        if (ti_do_statement(query, child, e))
            goto fail0;

        if (!ti_val_is_thing(query->rval))
        {
            ex_set(e, EX_TYPE_ERROR,
                    "expecting the first argument to be of "
                    "type `"TI_VAL_THING_S"` but got type `%s` instead",
                    ti_val_str(query->rval));
            goto fail0;
        }

        thing = (ti_thing_t *) query->rval;
        query->rval = NULL;

        deep_val = ti_thing_val_weak_by_name(thing, deep_name);
        load_val = ti_thing_val_weak_by_name(thing, load_name);

        if (deep_val && ti_deep_from_val(deep_val, &deep, e))
            goto fail1;

        if (load_val)
            load = ti_val_as_bool(load_val);

        child = child->next ? child->next->next : NULL;
    }
    else
    {
        thing = ti_thing_o_create(0, defaults_n, NULL);
        if (!thing)
        {
            ex_set_mem(e);
            goto fail0;
        }
        nargs = 1;
        child = NULL;
    }

    /*
     * Check here for if "args" is still NULL, since calls to ti_do_statement
     * might have changed the status.
     */
    if (future->args)
    {
        ex_set(e, EX_OPERATION,
                "future for module `%s` is pending "
                "and cannot be used a second time",
                module->name->str);
        goto fail1;
    }

    future->options = deep;
    if (load)
        future->options |= TI_FUTURE_LOAD_FLAG;

    future->args = vec_new(nargs);

    if (!future->args || ti_module_set_defaults(&thing, expose->defaults))
    {
        ex_set_mem(e);
        goto fail1;
    }

    VEC_push(future->args, thing);

    for (;child; child = child->next ? child->next->next : NULL)
    {
        if (ti_do_statement(query, child, e))
            goto fail1;

        VEC_push(future->args, query->rval);
        query->rval = NULL;
    }

    if (ti_future_register(future))
        ex_set_mem(e);

    goto done;

fail1:
    ti_val_gc_drop((ti_val_t *) thing);
fail0:
done:
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) future;
    return e->nr;
}

int ti_mod_expose_info_to_vp(
        const char * key,
        size_t n,
        ti_mod_expose_t * expose,
        ti_vp_t * vp)
{
    msgpack_packer * pk = &vp->pk;
    size_t defaults_n = (expose->defaults ? expose->defaults->n : 0) +\
            !!expose->deep +\
            !!expose->load;
    size_t sz = !!(expose->doc && *expose->doc) +\
            !!defaults_n +\
            !!expose->argmap +\
            !!expose->def +\
            !!expose->closure;

    if (mp_pack_strn(pk, key, n) ||
        msgpack_pack_map(pk, sz))
        return -1;

    if (expose->doc && *expose->doc && (
        mp_pack_str(pk, "doc") ||
        mp_pack_str(pk, expose->doc)))
        return -1;

    if (defaults_n)
    {
        if (mp_pack_str(pk, "defaults") ||
            msgpack_pack_map(pk, defaults_n))
            return -1;

        if (expose->deep && (
            mp_pack_str(pk, "deep") ||
            msgpack_pack_uint8(pk, *expose->deep)))
            return -1;

        if (expose->load && (
            mp_pack_str(pk, "load") ||
            mp_pack_bool(pk, *expose->load)))
            return -1;

        if (expose->defaults)
            for (vec_each(expose->defaults, ti_item_t, item))
                if (mp_pack_strn(pk, item->key->data, item->key->n) ||
                    ti_val_to_client_pk(item->val, vp, TI_MAX_DEEP, 0))
                    return -1;
    }

    if (expose->argmap)
    {
        if (mp_pack_str(pk, "argmap") ||
            msgpack_pack_array(pk, expose->argmap->n))
            return -1;

        for (vec_each(expose->argmap, ti_item_t, item))
            if (mp_pack_strn(pk, item->key->data, item->key->n))
                return -1;
    }

    if (expose->def && (
            mp_pack_str(pk, "definition") ||
            mp_pack_strn(pk, expose->def->data, expose->def->n)))
        return -1;

    if (expose->closure && (
            mp_pack_str(pk, "with_side_effects") ||
            mp_pack_bool(pk, expose->closure->flags & TI_CLOSURE_FLAG_WSE)))
        return -1;

    return 0;
}
