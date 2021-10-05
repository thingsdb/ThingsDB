/*
 * ti/mod/expose.c
 */
#include <ti/mod/expose.h>
#include <ti/val.inline.h>
#include <util/logger.h>
#include <ti/fn/fn.h>
#include <tiinc.h>

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
    vec_destroy(expose->argmap, (vec_destroy_cb) expose__item_destroy);
    vec_destroy(expose->defaults, (vec_destroy_cb) expose__item_destroy);
    free(expose);
}

int ti_mod_expose_call(
        ti_mod_expose_t * expose,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_future_t * future = (ti_future_t *) query->rval;
    cleri_children_t * child = nd->children;
    ti_module_t * module = future->module;
    size_t defaults_n = expose->defaults ? expose->defaults->n : 0;
    ti_thing_t * thing;

    if (ti.futures_count >= TI_MAX_FUTURE_COUNT)
    {
        ex_set(e, EX_MAX_QUOTA,
                "maximum number of active futures (%u) is reached",
                TI_MAX_FUTURE_COUNT);
        return e->nr;
    }

    if (future->args)
    {
        ex_set(e, EX_OPERATION,
                "future for module `%s` is pending "
                "and cannot be used a second time",
                module->name->str);
        return e->nr;
    }

    query->rval = NULL;

    if (expose->argmap)
    {
        size_t sz = expose->argmap->n + defaults_n;
        thing = ti_thing_o_create(0, sz, NULL);
        if (!thing)
        {
            ex_set_mem(e);
            goto fail0;
        }

        for (vec_each(expose->argmap, ti_raw_t, key))
        {
            if (child)
            {
                if (ti_do_statement(query, child->node, e))
                    goto fail1;

                if (child->next)
                    child = child->next->next;
            }
            else
            {
                query->rval = (ti_val_t *) ti_nil_get();
            }

            if (ti_thing_o_add(thing, key, query->rval))
            {
                ex_set_mem(e);
                goto fail1;
            }

            ti_incref(key);
            query->rval = NULL;
        }
    }
    else if (nargs)
    {
        if (ti_do_statement(query, child->node, e))
            goto fail0;

        if (!ti_val_is_thing(query->rval))
        {
            ex_set(e, EX_TYPE_ERROR,
                    "expecting the first argument to be of "
                    "type `"TI_VAL_THING_S"` but got type `%s` instead",
                    ti_val_str(query->rval));
            return e->nr;
        }
    }
    else
    {
        thing = ti_thing_o_create(0, defaults_n, NULL);
        if (!thing)
        {
            ex_set_mem(e);
            return e->nr;
        }
    }
    LOGC("mod: %p expose: %p, q: %p, nd: %p, e: %p",
            module, expose, query, nd, e);
    return 0;

fail1:
    ti_val_unsafe_drop((ti_val_t *) thing);
fail0:
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) future;
    return e->nr;
}

int ti_mod_expose_info_to_pk(
        const char * key,
        size_t n,
        ti_mod_expose_t * expose,
        msgpack_packer * pk)
{
    size_t defaults_n = (expose->defaults ? expose->defaults->n : 0) + \
            !!expose->deep + \
            !!expose->load;
    size_t sz = 1 + !!expose->argmap + !!expose->doc + !!(defaults_n);

    int rc = -(
            msgpack_pack_map(pk, sz) ||

            mp_pack_str(pk, "name") ||
            mp_pack_strn(pk, key, n) ||

            mp_pack_str(pk, "doc") ||
            mp_pack_str(pk, expose->doc)
    );
    /* TODO pack more... */
    return rc;
}
