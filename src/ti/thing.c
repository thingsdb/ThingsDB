/*
 * thing.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/prop.h>
#include <ti/proto.h>
#include <ti/thing.h>
#include <ti/val.h>
#include <util/qpx.h>
#include <util/logger.h>

static void thing__watch_del(ti_thing_t * thing);
static int thing__set(vec_t ** vec, ti_name_t * name, ti_val_t * val);

ti_thing_t * ti_thing_create(uint64_t id, imap_t * things)
{
    ti_thing_t * thing = malloc(sizeof(ti_thing_t));
    if (!thing)
        return NULL;

    thing->ref = 1;
    thing->tp = TI_VAL_THING;
    thing->flags = TI_THING_FLAG_SWEEP;

    thing->id = id;
    thing->things = things;
    thing->props = vec_new(0);
    thing->attrs = NULL;
    thing->watchers = NULL;

    if (id && ti_manages_id(id))
        ti_thing_mark_attrs(thing);

    if (!thing->props)
    {
        ti_thing_destroy(thing);
        return NULL;
    }
    return thing;
}

void ti_thing_destroy(ti_thing_t * thing)
{
    if (!thing)
        return;

    if (thing->id)
        (void *) imap_pop(thing->things, thing->id);

    if ((~ti()->flags & TI_FLAG_SIGNAL) && ti_thing_has_watchers(thing))
        thing__watch_del(thing);

    vec_destroy(thing->props, (vec_destroy_cb) ti_prop_destroy);
    vec_destroy(thing->attrs, (vec_destroy_cb) ti_prop_destroy);
    vec_destroy(thing->watchers, (vec_destroy_cb) ti_watch_drop);
    free(thing);
}

ti_val_t * ti_thing_prop_weak_get(ti_thing_t * thing, ti_name_t * name)
{
    for (vec_each(thing->props, ti_prop_t, prop))
        if (prop->name == name)
            return prop->val;
    return NULL;
}

ti_val_t * ti_thing_attr_weak_get(ti_thing_t * thing, ti_name_t * name)
{
    if (thing->attrs)
        for (vec_each(thing->attrs, ti_prop_t, attr))
            if (attr->name == name)
                return attr->val;

    return NULL;
}

/*
 * does not increment the `name` and `val` reference counters.
 */
int ti_thing_prop_set(ti_thing_t * thing, ti_name_t * name, ti_val_t * val)
{
    return thing__set(&thing->props, name, val);
}


int ti_thing_attr_set(ti_thing_t * thing, ti_name_t * name, ti_val_t * val)
{
    ti_prop_t * attr;

    if (!thing->attrs)
    {
        thing->attrs = vec_new(1);
        if (!thing->attrs)
            return -1;

        attr = ti_prop_weak_createv(name, val);
        if (!attr)
            return -1;

        VEC_push(thing->attrs, attr);
        return 0;
    }

    return thing__set(&thing->attrs, name, val);
}

/* Returns true if the property is removed, false if not found */
_Bool ti_thing_del(ti_thing_t * thing, ti_name_t * name)
{
    uint32_t i = 0;
    for (vec_each(thing->props, ti_prop_t, prop), ++i)
    {
        if (prop->name == name)
        {
            ti_prop_destroy(vec_remove(thing->props, i));
            return true;
        }
    }
    return false;
}

/* Returns true if the attribute is removed, false if not found */
_Bool ti_thing_unset(ti_thing_t * thing, ti_name_t * name)
{
    uint32_t i = 0;
    for (vec_each(thing->attrs, ti_prop_t, attr), ++i)
    {
        if (attr->name == name)
        {
            ti_prop_destroy(vec_remove(thing->attrs, i));
            return true;
        }
    }
    return false;
}

/*
 * Returns true if `from` is found and replaced by to, false if not found.
 * If found, then the `from` reference which was used by the thing will be
 * decremented, the reference count of `to` will never change so when using
 * this function you should act on the return value.
 */
_Bool ti_thing_rename(ti_thing_t * thing, ti_name_t * from, ti_name_t * to)
{
    uint32_t i = 0;
    for (vec_each(thing->props, ti_prop_t, prop), ++i)
    {
        if (prop->name == from)
        {
            ti_name_drop(prop->name);
            prop->name = to;
            return true;
        }
    }
    return false;
}

void ti_thing_attr_unset(ti_thing_t * thing, ti_name_t * name)
{
    uint32_t i = 0;
    if (!thing->attrs)
        return;

    for (vec_each(thing->attrs, ti_prop_t, attr), ++i)
    {
        if (attr->name == name)
        {
            ti_prop_destroy(vec_remove(thing->attrs, i));
            return;
        }
    }
}

int ti_thing_gen_id(ti_thing_t * thing)
{
    assert (!thing->id);

    thing->id = ti_next_thing_id();
    ti_thing_mark_new(thing);

    if (ti_manages_id(thing->id))
        ti_thing_mark_attrs(thing);

    if (ti_thing_to_map(thing))
        return -1;

    for (vec_each(thing->props, ti_prop_t, prop))
    {
        if (prop->val->tp == TI_VAL_THING)
        {
            ti_thing_t * tthing = (ti_thing_t *) prop->val;
            if (tthing->id)
            {
                ti_thing_unmark_new(tthing);
                continue;
            }

            if (ti_thing_gen_id(tthing))
                return -1;

            continue;
        }

        if (ti_val_is_list(prop->val))
        {
            ti_varr_t * varr = (ti_varr_t *) prop->val;
            for (vec_each(varr->vec, ti_thing_t, tthing))
            {
                if (tthing->tp != TI_VAL_THING)
                    continue;

                if (tthing->id)
                {
                    ti_thing_unmark_new(tthing);
                    continue;
                }

                if (ti_thing_gen_id(tthing))
                    return -1;
            }
        }
    }
    return 0;
}

ti_watch_t *  ti_thing_watch(ti_thing_t * thing, ti_stream_t * stream)
{
    ti_watch_t * watch;
    ti_watch_t ** empty_watch = NULL;
    if (!thing->watchers)
    {
        thing->watchers = vec_new(1);
        if (!thing->watchers)
            return NULL;
        watch = ti_watch_create(stream);
        if (!watch)
            return NULL;
        VEC_push(thing->watchers, watch);
        goto finish;
    }
    for (vec_each(thing->watchers, ti_watch_t, watch))
    {
        if (watch->stream == stream)
            return watch;
        if (!watch->stream)
            empty_watch = v__;
    }

    if (empty_watch)
    {
        watch = *empty_watch;
        watch->stream = stream;
        goto finish;
    }

    watch = ti_watch_create(stream);
    if (!watch)
        return NULL;

    if (vec_push(&thing->watchers, watch))
        goto fail0;

finish:
    if (!stream->watching)
    {
        stream->watching = vec_new(1);
        if (!stream->watching)
            goto fail1;
        VEC_push(stream->watching, watch);
    }
    else if (vec_push(&stream->watching, watch))
        goto fail1;

    return watch;

fail0:
    watch->stream = NULL;
    ti_watch_drop(watch);
    return NULL;
fail1:
    watch->stream = NULL;
    return NULL;
}

_Bool ti_thing_unwatch(ti_thing_t * thing, ti_stream_t * stream)
{
    if (!thing->watchers)
        return false;

    for (vec_each(thing->watchers, ti_watch_t, watch))
    {
        if (watch->stream == stream)
        {
            watch->stream = NULL;
            return true;
        }
    }
    return false;
}

int ti_thing_to_packer(ti_thing_t * thing, qp_packer_t ** packer, int flags)
{
    flags |= TI_VAL_PACK_THING;

    if (    qp_add_map(packer) ||
            qp_add_raw(*packer, (const uchar *) "#", 1) ||
            qp_add_int(*packer, thing->id))
        return -1;

    for (vec_each(thing->props, ti_prop_t, prop))
    {
        if (    qp_add_raw_from_str(*packer, prop->name->str) ||
                ti_val_to_packer(&prop->val, packer, flags))
            return -1;
    }

    if ((flags & TI_VAL_PACK_ATTR) && thing->attrs && thing->attrs->n)
    {
        assert (ti_thing_with_attrs(thing));

        if (qp_add_raw(*packer, (const uchar *) ".", 1) || qp_add_map(packer))
            return -1;

        for (vec_each(thing->attrs, ti_prop_t, prop))
        {
            if (    qp_add_raw_from_str(*packer, prop->name->str) ||
                    ti_val_to_packer(&prop->val, packer, flags))
                return -1;
        }

        if (qp_close_map(*packer))
            return -1;
    }


    return qp_close_map(*packer);
}

_Bool ti_thing_has_watchers(ti_thing_t * thing)
{
    if (!thing->watchers)
        return false;
    for (vec_each(thing->watchers, ti_watch_t, watch))
        if (watch->stream && (~watch->stream->flags & TI_STREAM_FLAG_CLOSED))
            return true;
    return false;
}

static void thing__watch_del(ti_thing_t * thing)
{
    assert (thing->watchers);

    ti_pkg_t * pkg;
    ti_rpkg_t * rpkg;
    qpx_packer_t * packer = qpx_packer_create(12, 1);
    if (!packer)
    {
        log_critical(EX_ALLOC_S);
        return;
    }
    (void) ti_thing_id_to_packer(thing, &packer);

    pkg = qpx_packer_pkg(packer, TI_PROTO_CLIENT_WATCH_DEL);

    rpkg = ti_rpkg_create(pkg);
    if (!rpkg)
    {
        log_critical(EX_ALLOC_S);
        return;
    }

    for (vec_each(thing->watchers, ti_watch_t, watch))
    {
        if (ti_stream_is_closed(watch->stream))
            continue;

        if (ti_stream_write_rpkg(watch->stream, rpkg))
            log_critical(EX_INTERNAL_S);
    }

    ti_rpkg_drop(rpkg);
}

static int thing__set(vec_t ** vec, ti_name_t * name, ti_val_t * val)
{
    ti_prop_t * prop;

    for (vec_each(*vec, ti_prop_t, p))
    {
        if (p->name == name)
        {
            ti_decref(name);
            ti_val_drop(p->val);
            p->val = val;
            return 0;
        }
    }

    prop = ti_prop_weak_createv(name, val);
    if (!prop || vec_push(vec, prop))
    {
        free(prop);
        return -1;
    }

    return 0;
}

