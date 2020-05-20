/*
 * thing.c
 */
#include <assert.h>
#include <doc.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/names.h>
#include <ti/procedures.h>
#include <ti/prop.h>
#include <ti/proto.h>
#include <ti/enums.h>
#include <ti/thing.h>
#include <ti/thing.inline.h>
#include <ti/val.h>
#include <ti/val.inline.h>
#include <util/logger.h>
#include <util/mpack.h>

static inline int thing__val_locked(
        ti_thing_t * thing,
        ti_name_t * name,
        ti_val_t * val,
        ex_t * e)
{
    /*
     * Array and Sets are the only two values with are mutable and not set
     * by reference (like things). An array is always type `list` since it
     * is a value attached to a `prop` type.
     */
    if (    (val->tp == TI_VAL_ARR || val->tp == TI_VAL_SET) &&
            (val->flags & TI_VFLAG_LOCK))
    {
        ex_set(e, EX_OPERATION_ERROR,
            "cannot change or remove property `%s` on "TI_THING_ID
            " while the `%s` is being used",
            name->str,
            thing->id,
            ti_val_str(val));
        return -1;
    }
    return 0;
}

static void thing__unwatch(ti_thing_t * thing, ti_stream_t * stream)
{
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    ti_pkg_t * pkg;

    if (ti_stream_is_closed(stream))
        return;

    if (mp_sbuffer_alloc_init(&buffer, 32, sizeof(ti_pkg_t)))
    {
        log_critical(EX_MEMORY_S);
        return;
    }
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    (void) ti_thing_id_to_pk(thing, &pk);

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, TI_PROTO_EV_ID, TI_PROTO_CLIENT_WATCH_STOP, buffer.size);

    if (ti_stream_write_pkg(stream, pkg))
        log_critical(EX_INTERNAL_S);
}

static void thing__watch_del(ti_thing_t * thing)
{
    assert (thing->watchers);

    msgpack_packer pk;
    msgpack_sbuffer buffer;
    ti_pkg_t * pkg;
    ti_rpkg_t * rpkg;

    if (mp_sbuffer_alloc_init(&buffer, 32, sizeof(ti_pkg_t)))
    {
        log_critical(EX_MEMORY_S);
        return;
    }
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    (void) ti_thing_id_to_pk(thing, &pk);

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, TI_PROTO_EV_ID, TI_PROTO_CLIENT_WATCH_DEL, buffer.size);

    rpkg = ti_rpkg_create(pkg);
    if (!rpkg)
    {
        free(pkg);
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

ti_thing_t * ti_thing_o_create(
        uint64_t id,
        size_t init_sz,
        ti_collection_t * collection)
{
    ti_thing_t * thing = malloc(sizeof(ti_thing_t));
    if (!thing)
        return NULL;

    thing->ref = 1;
    thing->tp = TI_VAL_THING;
    thing->flags = TI_VFLAG_THING_SWEEP;
    thing->type_id = TI_SPEC_OBJECT;

    thing->id = id;
    thing->collection = collection;
    thing->items = vec_new(init_sz);
    thing->watchers = NULL;

    if (!thing->items)
    {
        ti_thing_destroy(thing);
        return NULL;
    }
    return thing;
}

ti_thing_t * ti_thing_t_create(
        uint64_t id,
        ti_type_t * type,
        ti_collection_t * collection)
{
    ti_thing_t * thing = malloc(sizeof(ti_thing_t));
    if (!thing)
        return NULL;

    thing->ref = 1;
    thing->tp = TI_VAL_THING;
    thing->flags = TI_VFLAG_THING_SWEEP;
    thing->type_id = type->type_id;

    thing->id = id;
    thing->collection = collection;
    thing->items = vec_new(type->fields->n);
    thing->watchers = NULL;

    if (!thing->items)
    {
        ti_thing_destroy(thing);
        return NULL;
    }
    return thing;
}

static inline void thing__prop_destroy(ti_prop_t * prop)
{
    if (ti_val_is_array(prop->val))
        ((ti_varr_t *) prop->val)->parent = NULL;
    else if (ti_val_is_set(prop->val))
        ((ti_vset_t *) prop->val)->parent = NULL;
    ti_prop_destroy(prop);
}

static inline void thing__val_drop(ti_val_t * val)
{
    if (!val)
        return;
    if (--val->ref)
    {
        if (ti_val_is_array(val))
            ((ti_varr_t *) val)->parent = NULL;
        else if (ti_val_is_set(val))
            ((ti_vset_t *) val)->parent = NULL;
        return;
    }
    ti_val_destroy(val);
}

void ti_thing_destroy(ti_thing_t * thing)
{
    assert (thing);
    if (thing->id)
    {
        if (ti_events_cache_dropped_thing(thing))
            return;
        (void) imap_pop(thing->collection->things, thing->id);
    }

    if ((~ti.flags & TI_FLAG_SIGNAL) && ti_thing_has_watchers(thing))
        thing__watch_del(thing);

    /*
     * While dropping, mutable variable must clear the parent; for example
     *
     *   ({}.t = []).push(42);
     *
     * In this case the `thing` will be removed while the list stays alive.
     */
    vec_destroy(
            thing->items,
            ti_thing_is_object(thing)
                    ? (vec_destroy_cb) thing__prop_destroy
                    : (vec_destroy_cb) thing__val_drop);

    vec_destroy(thing->watchers, (vec_destroy_cb) ti_watch_drop);
    free(thing);
}

void ti_thing_clear(ti_thing_t * thing)
{
    if (ti_thing_is_object(thing))
    {
        ti_prop_t * prop;
        while ((prop = vec_pop(thing->items)))
            ti_prop_destroy(prop);
    }
    else
    {
        ti_val_t * val;
        while ((val = vec_pop(thing->items)))
            ti_val_drop(val);

        /* convert to a simple object since the thing is not type
         * compliant anymore */
        thing->type_id = TI_SPEC_OBJECT;
    }
}

int ti_thing_props_from_vup(
        ti_thing_t * thing,
        ti_vup_t * vup,
        size_t sz,
        ex_t * e)
{
    ti_val_t * val;
    ti_name_t * name;
    mp_obj_t mp_prop;
    while (sz--)
    {
        if (mp_next(vup->up, &mp_prop) != MP_STR)
        {
            ex_set(e, EX_TYPE_ERROR, "property names must be of type string");
            return e->nr;
        }

        if (!ti_name_is_valid_strn(mp_prop.via.str.data, mp_prop.via.str.n))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "property name must follow the naming rules"
                    DOC_NAMES);
            return e->nr;
        }

        name = ti_names_get(mp_prop.via.str.data, mp_prop.via.str.n);
        val = ti_val_from_vup_e(vup, e);

        if (!val || !name || ti_val_make_assignable(&val, thing, name, e) ||
            !ti_thing_o_prop_set(thing, name, val))
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

ti_thing_t * ti_thing_new_from_vup(ti_vup_t * vup, size_t sz, ex_t * e)
{
    ti_thing_t * thing;

    if (!vup->isclient)
    {
        ex_set(e, EX_BAD_DATA,
                "new things without an id can only be created from user input "
                "and are unexpected when parsed from node data");
        return NULL;
    }

    thing = ti_thing_o_create(0, sz, vup->collection);
    if (!thing)
    {
        ex_set_mem(e);
        return NULL;
    }

    if (ti_thing_props_from_vup(thing, vup, sz, e))
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
ti_prop_t * ti_thing_o_prop_add(
        ti_thing_t * thing,
        ti_name_t * name,
        ti_val_t * val)
{
    assert (ti_thing_is_object(thing));
    assert (ti_thing_o_prop_weak_get(thing, name) == NULL);

    ti_prop_t * prop = ti_prop_create(name, val);
    if (!prop || vec_push(&thing->items, prop))
    {
        free(prop);
        return NULL;
    }

    return prop;
}

/*
 * It takes a reference on `name` and `val` when successful
 */
static int thing_o__prop_set_e(
        ti_thing_t * thing,
        ti_name_t * name,
        ti_val_t * val,
        ex_t * e)
{
    ti_prop_t * prop;

    for (vec_each(thing->items, ti_prop_t, p))
    {
        if (p->name == name)
        {
            if (thing__val_locked(thing, p->name, p->val, e))
                return e->nr;

            ti_decref(name);
            ti_val_drop(p->val);
            p->val = val;

            return e->nr;
        }
    }

    prop = ti_prop_create(name, val);
    if (!prop || vec_push(&thing->items, prop))
    {
        free(prop);
        ex_set_mem(e);
    }

    return e->nr;
}

/*
 * Does not increment the `name` and `val` reference counters.
 */
ti_prop_t * ti_thing_o_prop_set(
        ti_thing_t * thing,
        ti_name_t * name,
        ti_val_t * val)
{
    assert (ti_thing_is_object(thing));
    ti_prop_t * prop;

    for (vec_each(thing->items, ti_prop_t, p))
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
    if (!prop || vec_push(&thing->items, prop))
    {
        free(prop);
        return NULL;
    }

    return prop;
}

/*
 * Does not increment the `name` and `val` reference counters.
 */
void ti_thing_t_prop_set(ti_thing_t * thing, ti_name_t * name, ti_val_t * val)
{
    ti_val_t ** vaddr = (ti_val_t **) vec_get_addr(
            thing->items,
            ti_field_by_name(ti_thing_type(thing), name)->idx);

    ti_val_drop(*vaddr);
    *vaddr = val;
}

/*
 * Return 0 if successful; This function makes a given `value` assignable so
 * it should not be used within a job.
 */
int ti_thing_o_set_val_from_valid_strn(
        ti_wprop_t * wprop,
        ti_thing_t * thing,
        const char * str,
        size_t n,
        ti_val_t ** val,
        ex_t * e)
{
    ti_name_t * name = ti_names_get(str, n);
    if (!name)
    {
        ex_set_mem(e);
        return e->nr;
    }

    if (ti_val_make_assignable(val, thing, name, e))
        return e->nr;

    if (thing_o__prop_set_e(thing, name, *val, e))
    {
        assert (e->nr);
        ti_name_drop(name);
        return e->nr;
    }

    ti_incref(*val);

    wprop->name = name;
    wprop->val = val;

    return 0;
}


/*
 * Return 0 if successful; This function makes a given `value` assignable so
 * it should not be used within a job.
 *
 * If successful, the reference counter of `val` will be increased
 */
int ti_thing_t_set_val_from_strn(
        ti_wprop_t * wprop,
        ti_thing_t * thing,
        const char * str,
        size_t n,
        ti_val_t ** val,
        ex_t * e)
{
    ti_val_t ** vaddr;
    ti_type_t * type = ti_thing_type(thing);
    ti_field_t * field = ti_field_by_strn_e(type, str, n, e);
    if (!field || ti_field_make_assignable(field, val, thing, e))
        return e->nr;

    vaddr = (ti_val_t **) vec_get_addr(thing->items, field->idx);
    if (thing__val_locked(thing, field->name, *vaddr, e))
        return e->nr;

    ti_val_drop(*vaddr);
    *vaddr = *val;

    ti_incref(*val);

    wprop->name = field->name;
    wprop->val = val;

    return 0;
}

/* Returns true if the property is removed, false if not found */
_Bool ti_thing_o_del(ti_thing_t * thing, ti_name_t * name)
{
    assert (ti_thing_is_object(thing));

    uint32_t i = 0;
    for (vec_each(thing->items, ti_prop_t, prop), ++i)
    {
        if (prop->name == name)
        {
            ti_prop_destroy(vec_swap_remove(thing->items, i));
            return true;
        }
    }
    return false;
}

/* Returns 0 if the property is removed, -1 in case of an error */
int ti_thing_o_del_e(ti_thing_t * thing, ti_raw_t * rname, ex_t * e)
{
    assert (ti_thing_is_object(thing));

    uint32_t i = 0;
    ti_name_t * name = ti_names_weak_from_raw(rname);

    if (name)
    {
        for (vec_each(thing->items, ti_prop_t, prop), ++i)
        {
            if (prop->name == name)
            {
                if (thing__val_locked(thing, prop->name, prop->val, e))
                    return e->nr;

                ti_prop_destroy(vec_swap_remove(thing->items, i));
                return 0;
            }
        }
    }

    ti_thing_set_not_found(thing, name, rname, e);
    return e->nr;
}

static _Bool thing_o__get_by_name(
        ti_wprop_t * wprop,
        ti_thing_t * thing,
        ti_name_t * name)
{
    for (vec_each(thing->items, ti_prop_t, prop))
    {
        if (prop->name == name)
        {
            wprop->name = name;
            wprop->val = &prop->val;
            return true;
        }
    }
    return false;
}

static _Bool thing_t__get_by_name(
        ti_wprop_t * wprop,
        ti_thing_t * thing,
        ti_name_t * name)
{
    ti_name_t * n;
    ti_val_t ** v;
    for (thing_t_each_addr(thing, n, v))
    {
        if (n == name)
        {
            wprop->name = name;
            wprop->val = v;
            return true;
        }
    }
    return false;
}

_Bool ti_thing_get_by_raw(ti_wprop_t * wprop, ti_thing_t * thing, ti_raw_t * r)
{
    ti_name_t * name = r->tp == TI_VAL_NAME
            ? (ti_name_t *) r
            : ti_names_weak_from_raw(r);
    return name && (ti_thing_is_object(thing)
            ? thing_o__get_by_name(wprop, thing, name)
            : thing_t__get_by_name(wprop, thing, name));
}

int ti_thing_get_by_raw_e(
        ti_wprop_t * wprop,
        ti_thing_t * thing,
        ti_raw_t * r,
        ex_t * e)
{
    ti_name_t * name = ti_names_weak_from_raw(r);
    if (!name || (ti_thing_is_object(thing)
            ? !thing_o__get_by_name(wprop, thing, name)
            : !thing_t__get_by_name(wprop, thing, name)))
        ti_thing_set_not_found(thing, name, r, e);

    return e->nr;
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
    if (ti_thing_is_object(thing))
    {
        for (vec_each(thing->items, ti_prop_t, prop))
            if (ti_val_gen_ids(prop->val))
                return -1;
    }
    else
    {
        for (vec_each(thing->items, ti_val_t, val))
            if (ti_val_gen_ids(val))
                return -1;
    }
    return 0;
}

ti_watch_t * ti_thing_watch(ti_thing_t * thing, ti_stream_t * stream)
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
    for (vec_each_addr(thing->watchers, ti_watch_t, watch))
    {
        if ((*watch)->stream == stream)
            return *watch;
        if (!(*watch)->stream)
            empty_watch = watch;
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

int ti_thing_watch_init(ti_thing_t * thing, ti_stream_t * stream)
{
    ti_pkg_t * pkg;
    vec_t * pkgs_queue;
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    ti_collection_t * collection = thing->collection;
    _Bool is_collection = thing == collection->root;
     ti_watch_t * watch = ti_thing_watch(thing, stream);
    if (!watch)
        return -1;

    if (mp_sbuffer_alloc_init(&buffer, 8192, sizeof(ti_pkg_t)))
        return -1;

    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_map(&pk, is_collection ? 6 : 3);

    mp_pack_str(&pk, "event");
    msgpack_pack_uint64(&pk, ti.node->cevid);

    mp_pack_str(&pk, "thing");

    if (ti_thing__to_pk(thing, &pk, TI_VAL_PACK_TASK /* options */) ||
        mp_pack_str(&pk, "collection") ||
        mp_pack_strn(&pk, collection->name->data, collection->name->n))
    {
        msgpack_sbuffer_destroy(&buffer);
        return -1;
    }

    if (is_collection && (
            mp_pack_str(&pk, "enums") ||
            ti_enums_to_pk(collection->enums, &pk) ||
            mp_pack_str(&pk, "types") ||
            ti_types_to_pk(collection->types, &pk) ||
            mp_pack_str(&pk, "procedures") ||
            ti_procedures_to_pk(collection->procedures, &pk)))
    {
        msgpack_sbuffer_destroy(&buffer);
        return -1;
    }

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, TI_PROTO_EV_ID, TI_PROTO_CLIENT_WATCH_INI, buffer.size);

    if (ti_stream_is_closed(stream) ||
        ti_stream_write_pkg(stream, pkg))
        free(pkg);

    pkgs_queue = ti_events_pkgs_from_queue(thing);
    if (pkgs_queue)
    {
        for (vec_each(pkgs_queue, ti_pkg_t, pkg))
        {
            if (ti_stream_is_closed(stream) ||
                ti_stream_write_pkg(stream, pkg))
                free(pkg);
        }
        vec_destroy(pkgs_queue, NULL);
    }
    return 0;
}

int ti_thing_unwatch(ti_thing_t * thing, ti_stream_t * stream)
{
    size_t idx = 0;
    if (!thing->watchers)
        return 0;

    for (vec_each(thing->watchers, ti_watch_t, watch), ++idx)
    {
        if (watch->stream == stream)
        {
            watch->stream = NULL;
            vec_swap_remove(thing->watchers, idx);
            thing__unwatch(thing, stream);
            return 0;
        }
    }
    return 0;
}

static ti_pkg_t * thing__fwd(
        ti_thing_t * thing,
        uint16_t pkg_id,
        ti_proto_enum_t proto)
{
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    ti_pkg_t * pkg;

    if (mp_sbuffer_alloc_init(&buffer, 64, sizeof(ti_pkg_t)))
        return NULL;

    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);
    msgpack_pack_array(&pk, 2);
    msgpack_pack_uint64(&pk, thing->collection->root->id);
    msgpack_pack_uint64(&pk, thing->id);

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, pkg_id, proto, buffer.size);

    return pkg;
}

int ti_thing_watch_fwd(
        ti_thing_t * thing,
        ti_stream_t * stream,
        uint16_t pkg_id)
{
    ti_pkg_t * pkg = thing__fwd(thing, pkg_id, TI_PROTO_NODE_FWD_WATCH);
    return pkg ? ti_stream_write_pkg(stream, pkg) : -1;
}

int ti_thing_unwatch_fwd(
        ti_thing_t * thing,
        ti_stream_t * stream,
        uint16_t pkg_id)
{
    ti_pkg_t * pkg = thing__fwd(thing, pkg_id, TI_PROTO_NODE_FWD_UNWATCH);
    return pkg ? ti_stream_write_pkg(stream, pkg) : -1;
}

void ti_thing_t_to_object(ti_thing_t * thing)
{
    assert (!ti_thing_is_object(thing));
    ti_name_t * name;
    ti_val_t ** val;
    ti_prop_t * prop;
    for (thing_t_each_addr(thing, name, val))
    {
        prop = ti_prop_create(name, *val);
        if (!prop)
            ti_panic("cannot recover from a state between object and instance");

        ti_incref(name);
        *val = (ti_val_t *) prop;
    }
    thing->type_id = TI_SPEC_OBJECT;
}

int ti_thing__to_pk(ti_thing_t * thing, msgpack_packer * pk, int options)
{
    assert (options);  /* should be either positive or negative, not 0 */

    if (msgpack_pack_map(pk, (!!thing->id) + thing->items->n))
        return -1;

    if (thing->id && (
            mp_pack_strn(pk, TI_KIND_S_THING, 1) ||
            msgpack_pack_uint64(pk, thing->id)
    )) return -1;

    if (options > 0)
        --options;

    thing->flags |= TI_VFLAG_LOCK;

    if (ti_thing_is_object(thing))
    {
        for (vec_each(thing->items, ti_prop_t, prop))
        {
            if (mp_pack_strn(pk, prop->name->str, prop->name->n) ||
                ti_val_to_pk(prop->val, pk, options)
            ) goto fail;
        }
    }
    else
    {
        ti_name_t * name;
        ti_val_t * val;
        for (thing_t_each(thing, name, val))
        {
            if (mp_pack_strn(pk, name->str, name->n) ||
                ti_val_to_pk(val, pk, options)
            ) goto fail;
        }
    }

    thing->flags &= ~TI_VFLAG_LOCK;
    return 0;
fail:
    thing->flags &= ~TI_VFLAG_LOCK;
    return -1;
}

int ti_thing_t_to_pk(ti_thing_t * thing, msgpack_packer * pk, int options)
{
    assert (options < 0);  /* should only be called when options < 0 */
    assert (!ti_thing_is_object(thing));
    assert (thing->id);   /* no need to check, options < 0 must have id */

    if (msgpack_pack_map(pk, 3) ||
        mp_pack_strn(pk, TI_KIND_S_INSTANCE, 1) ||
        msgpack_pack_uint16(pk, thing->type_id) ||
        mp_pack_strn(pk, TI_KIND_S_THING, 1) ||
        msgpack_pack_uint64(pk, thing->id) ||
        msgpack_pack_str(pk, 0) ||
        msgpack_pack_array(pk, thing->items->n))
        return -1;

    for (vec_each(thing->items, ti_val_t, val))
        if (ti_val_to_pk(val, pk, options))
            return -1;

    return 0;
}

_Bool ti__thing_has_watchers_(ti_thing_t * thing)
{
    assert (thing->watchers);
    for (vec_each(thing->watchers, ti_watch_t, watch))
        if (watch->stream && (~watch->stream->flags & TI_STREAM_FLAG_CLOSED))
            return true;
    return false;
}
