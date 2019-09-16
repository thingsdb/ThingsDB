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
#include <ti/names.h>
#include <util/qpx.h>
#include <util/logger.h>

static inline int thing__prop_locked(
        ti_thing_t * thing,
        ti_prop_t * prop,
        ex_t * e)
{
    /*
     * Array and Sets are the only two values with are mutable and not set
     * by reference (like things). An array is always type `list` since it
     * is a value attached to a `prop` type.
     */
    if (    (prop->val->tp == TI_VAL_ARR || prop->val->tp == TI_VAL_SET) &&
            (prop->val->flags & TI_VFLAG_LOCK))
    {
        ex_set(e, EX_BAD_DATA,
            "cannot change or remove property `%s` on "TI_THING_ID
            " while the `%s` is being used",
            prop->name->str,
            thing->id,
            ti_val_str(prop->val));
        return -1;
    }
    return 0;
}

static void thing__watch_del(ti_thing_t * thing)
{
    assert (thing->watchers);

    ti_pkg_t * pkg;
    ti_rpkg_t * rpkg;
    qpx_packer_t * packer = qpx_packer_create(12, 1);
    if (!packer)
    {
        log_critical(EX_MEMORY_S);
        return;
    }
    (void) ti_thing_id_to_packer(thing, &packer);

    pkg = qpx_packer_pkg(packer, TI_PROTO_CLIENT_WATCH_DEL);

    rpkg = ti_rpkg_create(pkg);
    if (!rpkg)
    {
        log_critical(EX_MEMORY_S);
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

ti_thing_t * ti_thing_create(uint64_t id, imap_t * things)
{
    ti_thing_t * thing = malloc(sizeof(ti_thing_t));
    if (!thing)
        return NULL;

    thing->ref = 1;
    thing->tp = TI_VAL_THING;
    thing->flags = TI_VFLAG_THING_SWEEP;

    thing->id = id;
    thing->things = things;
    thing->props = vec_new(0);
    thing->watchers = NULL;

    if (!thing->props)
    {
        ti_thing_destroy(thing);
        return NULL;
    }
    return thing;
}

void ti_thing_destroy(ti_thing_t * thing)
{
    assert (thing);
    if (thing->id)
    {
        if (ti_events_cache_dropped_thing(thing))
            return;
        (void) imap_pop(thing->things, thing->id);
    }

    if ((~ti()->flags & TI_FLAG_SIGNAL) && ti_thing_has_watchers(thing))
        thing__watch_del(thing);

    vec_destroy(thing->props, (vec_destroy_cb) ti_prop_destroy);
    vec_destroy(thing->watchers, (vec_destroy_cb) ti_watch_drop);
    free(thing);
}

void ti_thing_clear(ti_thing_t * thing)
{
    ti_prop_t * prop;
    while ((prop = vec_pop(thing->props)))
        ti_prop_destroy(prop);
}

int ti_thing_props_from_unp(
        ti_thing_t * thing,
        imap_t * things,
        qp_unpacker_t * unp,
        ssize_t sz,
        ex_t * e)
{
    while (sz--)
    {
        ti_val_t * val;
        ti_name_t * name;
        qp_obj_t qp_prop;
        if (qp_is_close(qp_next(unp, &qp_prop)))
            return e->nr;

        if (!qp_is_raw(qp_prop.tp))
        {
            ex_set(e, EX_BAD_DATA, "property names must be of type string");
            return e->nr;
        }

        if (!ti_name_is_valid_strn((const char *) qp_prop.via.raw, qp_prop.len))
        {
            ex_set(e, EX_BAD_DATA,
                    "property name must follow the naming rules"
                    TI_SEE_DOC("#names"));
            return e->nr;
        }

        name = ti_names_get((const char *) qp_prop.via.raw, qp_prop.len);
        val = ti_val_from_unp_e(unp, things, e);

        if (!val || !name || !ti_thing_prop_set(thing, name, val))
        {
            if (!e->nr)
                ex_set_mem(e);
            ti_val_drop(val);
            ti_name_drop(name);
            return e->nr;
        }
    }
    return e->nr;
}

ti_thing_t * ti_thing_new_from_unp(
        qp_unpacker_t * unp,
        imap_t * things,        /* may be NULL */
        ssize_t sz,             /* size, or -1 when MAP_OPEN */
        ex_t * e)
{
    ti_thing_t * thing;

    if (~unp->flags & TI_VAL_UNP_FROM_CLIENT)
    {
        ex_set(e, EX_BAD_DATA,
                "new things without an id can only be created from user input "
                "and are unexpected when parsed from node data");
        return NULL;
    }

    thing = ti_thing_create(0, things);
    if (!thing)
    {
        ex_set_mem(e);
        return NULL;
    }

    if (ti_thing_props_from_unp(thing, things, unp, sz, e))
    {
        ti_val_drop((ti_val_t *) thing);
        return NULL;  /* error is set */
    }

    return thing;
}

/*
 * Does not increment the `name` and `val` reference counters.
 * Use only when you are sure the property does not yet exist.
 */
ti_prop_t * ti_thing_prop_add(ti_thing_t * thing, ti_name_t * name, ti_val_t * val)
{
    assert (ti_thing_prop_weak_get(thing, name) == NULL);

    ti_prop_t * prop = ti_prop_create(name, val);
    if (!prop || vec_push(&thing->props, prop))
    {
        free(prop);
        return NULL;
    }

    return prop;
}

/*
 * Does not increment the `name` and `val` reference counters.
 */
ti_prop_t * ti_thing_prop_set(ti_thing_t * thing, ti_name_t * name, ti_val_t * val)
{
    ti_prop_t * prop;

    for (vec_each(thing->props, ti_prop_t, p))
    {
        if (p->name == name)
        {
            ti_decref(name);
            ti_val_drop(p->val);
            p->val = val;
            return p;
        }
    }

    prop = ti_prop_create(name, val);
    if (!prop || vec_push(&thing->props, prop))
    {
        free(prop);
        return NULL;
    }

    return prop;
}

/*
 * Does not increment the `name` and `val` reference counters.
 */
ti_prop_t * ti_thing_prop_set_e(
        ti_thing_t * thing,
        ti_name_t * name,
        ti_val_t * val,
        ex_t * e)
{
    ti_prop_t * prop;

    for (vec_each(thing->props, ti_prop_t, p))
    {
        if (p->name == name)
        {
            if (thing__prop_locked(thing, p, e))
                return NULL;
            ti_decref(name);
            ti_val_drop(p->val);
            p->val = val;
            return p;
        }
    }

    prop = ti_prop_create(name, val);
    if (!prop || vec_push(&thing->props, prop))
    {
        free(prop);
        return NULL;
    }

    return prop;
}

/* Returns true if the property is removed, false if not found */
_Bool ti_thing_del(ti_thing_t * thing, ti_name_t * name)
{
    uint32_t i = 0;
    for (vec_each(thing->props, ti_prop_t, prop), ++i)
    {
        if (prop->name == name)
        {
            ti_prop_destroy(vec_swap_remove(thing->props, i));
            return true;
        }
    }
    return false;
}

static inline void thing__set_not_found(
        ti_thing_t * thing,
        ti_name_t * name,
        ti_raw_t * rname,
        ex_t * e)
{
    if (name || ti_name_is_valid_strn((const char *) rname->data, rname->n))
    {
        ex_set(e, EX_INDEX_ERROR,
                "thing "TI_THING_ID" has no property `%.*s`",
                thing->id,
                (int) rname->n, (const char *) rname->data);
    }
    else
    {
        ex_set(e, EX_BAD_DATA,
                "property name must follow the naming rules"
                TI_SEE_DOC("#names"));
    }
}

/* Returns 0 if the property is removed, -1 in case of an error */
int ti_thing_del_e(ti_thing_t * thing, ti_raw_t * rname, ex_t * e)
{
    uint32_t i = 0;
    ti_name_t * name = ti_names_weak_get((const char *) rname->data, rname->n);

    if (name)
    {
        for (vec_each(thing->props, ti_prop_t, prop), ++i)
        {
            if (prop->name == name)
            {
                if (thing__prop_locked(thing, prop, e))
                    return e->nr;

                ti_prop_destroy(vec_swap_remove(thing->props, i));
                return 0;
            }
        }
    }

    thing__set_not_found(thing, name, rname, e);
    return e->nr;
}

ti_prop_t * ti_thing_weak_get(ti_thing_t * thing, ti_raw_t * rname)
{
    ti_name_t * name = ti_names_weak_get((const char *) rname->data, rname->n);

    if (name)
        for (vec_each(thing->props, ti_prop_t, prop))
            if (prop->name == name)
                return prop;

    return NULL;
}

ti_prop_t * ti_thing_weak_get_e(ti_thing_t * thing, ti_raw_t * rname, ex_t * e)
{
    ti_name_t * name = ti_names_weak_get((const char *) rname->data, rname->n);

    if (name)
        for (vec_each(thing->props, ti_prop_t, prop))
            if (prop->name == name)
                return prop;

    thing__set_not_found(thing, name, rname, e);
    return NULL;
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

int ti_thing_gen_id(ti_thing_t * thing)
{
    assert (!thing->id);

    thing->id = ti_next_thing_id();
    ti_thing_mark_new(thing);

    if (ti_thing_to_map(thing))
        return -1;

    /*
     * Recursion is required since nested things did not generate a task
     * as long as their parent was not attached to the collection.
     */
    for (vec_each(thing->props, ti_prop_t, prop))
        if (ti_val_gen_ids(prop->val))
            return -1;

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
        goto failed;

finish:
    if (!stream->watching)
    {
        stream->watching = vec_new(1);
        if (!stream->watching)
            goto failed;
        VEC_push(stream->watching, watch);
    }
    else if (vec_push(&stream->watching, watch))
        goto failed;

    return watch;

failed:
    /* when this fails, a few bytes might leak */
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

int ti_thing_to_packer(ti_thing_t * thing, qp_packer_t ** packer, int options)
{
    assert (options);  /* should be either positive or negative, not 0 */

    if (qp_add_map(packer))
        return -1;

    if (thing->id && (
            qp_add_raw(*packer, (const uchar *) TI_KIND_S_THING, 1) ||
            qp_add_int(*packer, thing->id)))
        return -1;

    if (thing->flags & TI_VFLAG_LOCK)
        goto stop;  /* no nesting */

    --options;

    thing->flags |= TI_VFLAG_LOCK;

    for (vec_each(thing->props, ti_prop_t, prop))
    {
        if (    qp_add_raw_from_str(*packer, prop->name->str) ||
                ti_val_to_packer(prop->val, packer, options))
        {
            thing->flags &= ~TI_VFLAG_LOCK;
            return -1;
        }
    }

    thing->flags &= ~TI_VFLAG_LOCK;

stop:
    return qp_close_map(*packer);
}

_Bool ti__thing_has_watchers_(ti_thing_t * thing)
{
    assert (thing->watchers);
    for (vec_each(thing->watchers, ti_watch_t, watch))
        if (watch->stream && (~watch->stream->flags & TI_STREAM_FLAG_CLOSED))
            return true;
    return false;
}
