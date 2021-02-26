/*
 * thing.c
 */
#include <assert.h>
#include <doc.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/enums.h>
#include <ti/events.inline.h>
#include <ti/field.h>
#include <ti/item.h>
#include <ti/item.t.h>
#include <ti/method.h>
#include <ti/names.h>
#include <ti/opr.h>
#include <ti/procedures.h>
#include <ti/prop.h>
#include <ti/proto.h>
#include <ti/raw.inline.h>
#include <ti/thing.h>
#include <ti/thing.inline.h>
#include <ti/types.h>
#include <ti/val.h>
#include <ti/val.inline.h>
#include <ti/watch.h>
#include <util/logger.h>
#include <util/mpack.h>

static vec_t * thing__gc_swp;
vec_t * ti_thing_gc_vec;


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
        ex_set(e, EX_OPERATION,
            "cannot change or remove property `%s` on "TI_THING_ID
            " while the `%s` is being used",
            name->str,
            thing->id,
            ti_val_str(val));
        return -1;
    }
    return 0;
}

static inline int thing__item_val_locked(
        ti_thing_t * thing,
        ti_raw_t * key,
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
        ex_set(e, EX_OPERATION,
            "cannot change or remove property `%s` on "TI_THING_ID
            " while the `%s` is being used",
            ti_raw_as_printable_str(key),
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
    thing->flags = TI_THING_FLAG_SWEEP;
    thing->type_id = TI_SPEC_OBJECT;

    thing->id = id;
    thing->collection = collection;
    thing->items.vec = vec_new(init_sz);
    thing->watchers = NULL;

    if (!thing->items.vec)
    {
        ti_thing_destroy(thing);
        return NULL;
    }
    return thing;
}

ti_thing_t * ti_thing_i_create(uint64_t id, ti_collection_t * collection)
{
    ti_thing_t * thing = malloc(sizeof(ti_thing_t));
    if (!thing)
        return NULL;

    thing->ref = 1;
    thing->tp = TI_VAL_THING;
    thing->flags = TI_THING_FLAG_SWEEP|TI_THING_FLAG_DICT;
    thing->type_id = TI_SPEC_OBJECT;

    thing->id = id;
    thing->collection = collection;
    thing->items.smap = smap_create();
    thing->watchers = NULL;

    if (!thing->items.smap)
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
    thing->flags = TI_THING_FLAG_SWEEP;
    thing->type_id = type->type_id;

    thing->id = id;
    thing->collection = collection;
    thing->items.vec = vec_new(type->fields->n);
    thing->watchers = NULL;

    if (!thing->items.vec)
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

        (void) imap_pop(thing->collection->things, thing->id);
        /*
         * It is not possible that the thing exist in garbage collection
         * since the garbage collector hold a reference to the thing and
         * will therefore never destroy.
         */
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
    if (ti_thing_is_dict(thing))
        smap_destroy(thing->items.smap, (smap_destroy_cb) ti_item_destroy);
    else
        vec_destroy(thing->items.vec, ti_thing_is_object(thing)
                ? (vec_destroy_cb) ti_prop_destroy
                : (vec_destroy_cb) ti_val_unassign_drop);

    vec_destroy(thing->watchers, (vec_destroy_cb) ti_watch_drop);
    free(thing);
}

void ti_thing_clear(ti_thing_t * thing)
{
    if (ti_thing_is_object(thing))
    {
        if (ti_thing_is_dict(thing))
        {
            smap_clear(thing->items.smap, (smap_destroy_cb) ti_item_destroy);
        }
        else
        {
            ti_prop_t * prop;
            while ((prop = vec_pop(thing->items.vec)))
                ti_prop_destroy(prop);
        }
    }
    else
    {
        ti_val_t * val;
        while ((val = vec_pop(thing->items.vec)))
            ti_val_unsafe_gc_drop(val);

        /* convert to a simple object since the thing is not type
         * compliant anymore */
        thing->type_id = TI_SPEC_OBJECT;
    }
}

int ti_thing_to_dict(ti_thing_t * thing)
{
    smap_t * smap = smap_create();
    if (!smap)
        return -1;

    for (vec_each(thing->items.vec, ti_prop_t, prop))
        if (smap_add(smap, prop->name->str, prop))
            goto fail0;

    thing->flags |= TI_THING_FLAG_DICT;
    free(thing->items.vec);
    thing->items.smap = smap;
    return 0;

fail0:
    smap_destroy(smap, NULL);
    return -1;
}

typedef struct
{
    ti_raw_t ** incompatible;
    vec_t * vec;
} thing__to_stric_t;

static int thing__to_strict_cb(ti_item_t * item, thing__to_stric_t * w)
{
    if (ti_raw_is_name(item->key))
    {
        VEC_push(w->vec, item);
        return 0;
    }
    *w->incompatible = item->key;
    return -1;
}

int ti_thing_to_strict(ti_thing_t * thing, ti_raw_t ** incompatible)
{
    vec_t * vec = vec_new(ti_thing_n(thing));
    if (!vec)
    {
        *incompatible = NULL;
        return -1;
    }
    thing__to_stric_t w = {
            .incompatible = incompatible,
            .vec = vec,
    };
    if (smap_values(
            thing->items.smap,
            (smap_val_cb) thing__to_strict_cb,
            &w))
    {
        free(vec);
        return -1;
    }

    smap_destroy(thing->items.smap, NULL);
    thing->items.vec = vec;
    thing->flags &= ~TI_THING_FLAG_DICT;
    return 0;
}

int ti_thing_props_from_vup(
        ti_thing_t * thing,
        ti_vup_t * vup,
        size_t sz,
        ex_t * e)
{
    ti_val_t * val;
    ti_raw_t * key;
    mp_obj_t mp_prop;
    while (sz--)
    {
        if (mp_next(vup->up, &mp_prop) != MP_STR)
        {
            ex_set(e, EX_TYPE_ERROR, "property names must be of type string");
            return e->nr;
        }

        if (!strx_is_utf8n(mp_prop.via.str.data, mp_prop.via.str.n))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "properties must have valid UTF-8 encoding");
            return e->nr;
        }

        if (ti_is_reserved_key_strn(mp_prop.via.str.data, mp_prop.via.str.n))
        {
            ex_set(e, EX_VALUE_ERROR, "property `%c` is reserved"DOC_PROPERTIES,
                    *mp_prop.via.str.data);
            return e->nr;
        }

        key = ti_name_is_valid_strn(mp_prop.via.str.data, mp_prop.via.str.n)
            ? (ti_raw_t *) ti_names_get(mp_prop.via.str.data, mp_prop.via.str.n)
            : ti_str_create(mp_prop.via.str.data, mp_prop.via.str.n);

        val = ti_val_from_vup_e(vup, e);

        if (!val || !key || ti_val_make_assignable(&val, thing, key, e) ||
            ti_thing_o_set(thing, key, val))
        {
            if (!e->nr)
                ex_set_mem(e);
            ti_val_drop(val);
            ti_val_drop((ti_val_t *) key);
            return e->nr;
        }
    }
    return e->nr;
}

ti_thing_t * ti_thing_new_from_vup(ti_vup_t * vup, size_t sz, ex_t * e)
{
    ti_thing_t * thing;

    if (!vup->isclient && vup->collection)
    {
        ex_set(e, EX_BAD_DATA,
                "new things without an id can only be created from user input "
                "or in the thingsdb scope, and are unexpected when parsed "
                "from node data with a collection");
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
        ti_val_unsafe_drop((ti_val_t *) thing);
        return NULL;  /* error is set */
    }

    return thing;
}

/*
 * Does not increment the `name` and `val` reference counters.
 * Use only when you are sure the property does not yet exist.
 */
ti_prop_t * ti_thing_p_prop_add(
        ti_thing_t * thing,
        ti_name_t * name,
        ti_val_t * val)
{
    ti_prop_t * prop = ti_prop_create(name, val);
    if (!prop || vec_push(&thing->items.vec, prop))
    {
        free(prop);
        return NULL;
    }
    return prop;
}

/*
 * Does not increment the `key` and `val` reference counters.
 * Use only when you are sure the property does not yet exist.
 */
ti_item_t * ti_thing_i_item_add(
        ti_thing_t * thing,
        ti_raw_t * key,
        ti_val_t * val)
{
    ti_item_t * item = ti_item_create(key, val);
    if (!item || smap_addn(
            thing->items.smap,
            (const char *) key->data,
            key->n,
            item))
    {
        free(item);
        return NULL;
    }
    return item;
}


/*
 * It takes a reference on `name` and `val` when successful
 */
static int thing_p__prop_set_e(
        ti_thing_t * thing,
        ti_name_t * name,
        ti_val_t * val,
        ex_t * e)
{
    ti_prop_t * prop;

    for (vec_each(thing->items.vec, ti_prop_t, p))
    {
        if (p->name == name)
        {
            if (thing__val_locked(thing, p->name, p->val, e))
                return e->nr;

            ti_decref(name);
            ti_val_unsafe_gc_drop(p->val);
            p->val = val;

            return e->nr;
        }
    }

    prop = ti_prop_create(name, val);
    if (!prop || vec_push(&thing->items.vec, prop))
    {
        free(prop);
        ex_set_mem(e);
    }

    return e->nr;
}

/*
 * It takes a reference on `name` and `val` when successful
 */
static int thing_i__item_set_e(
        ti_thing_t * thing,
        ti_raw_t * key,
        ti_val_t * val,
        ex_t * e)
{
    ti_item_t * item = smap_getn(
            thing->items.smap,
            (const char *)
            key->data, key->n);
    if (item)
    {
        if (thing__item_val_locked(thing, item->key, item->val, e))
            return e->nr;
        ti_val_unsafe_drop((ti_val_t *) item->key);
        ti_val_unsafe_gc_drop(item->val);
        item->val = val;
        item->key = key;
        return e->nr;
    }

    item = ti_item_create(key, val);
    if (smap_addn(thing->items.smap, (const char *) key->data, key->n, item))
    {
        free(item);
        ex_set_mem(e);
    }

    return e->nr;
}

/*
 * Does not increment the `name` and `val` reference counters.
 */
ti_prop_t * ti_thing_p_prop_set(
        ti_thing_t * thing,
        ti_name_t * name,
        ti_val_t * val)
{
    ti_prop_t * prop;

    for (vec_each(thing->items.vec, ti_prop_t, p))
    {
        if (p->name == name)
        {
            ti_decref(name);
            ti_val_unsafe_gc_drop(p->val);
            p->val = val;
            return p;
        }
    }

    prop = ti_prop_create(name, val);
    if (!prop || vec_push(&thing->items.vec, prop))
    {
        free(prop);
        return NULL;
    }

    return prop;
}

/*
 * Does not increment the `name` and `val` reference counters.
 */
ti_item_t * ti_thing_i_item_set(
        ti_thing_t * thing,
        ti_raw_t * key,
        ti_val_t * val)
{
    ti_item_t * item = smap_getn(
            thing->items.smap,
            (const char *)
            key->data, key->n);
    if (item)
    {
        ti_val_unsafe_drop((ti_val_t *) item->key);
        ti_val_unsafe_gc_drop(item->val);
        item->val = val;
        item->key = key;
        return item;
    }

    item = ti_item_create(key, val);
    if (smap_addn(thing->items.smap, (const char *) key->data, key->n, item))
    {
        free(item);
        return NULL;
    }

    return item;
}

/*
 * Does not increment the `val` reference counters.
 */
void ti_thing_t_prop_set(
        ti_thing_t * thing,
        ti_field_t * field,
        ti_val_t * val)
{
    ti_val_t ** vaddr = (ti_val_t **) vec_get_addr(
            thing->items.vec,
            field->idx);
    ti_val_unsafe_gc_drop(*vaddr);
    *vaddr = val;
}

int ti_thing_i_set_val_from_strn(
        ti_witem_t * witem,
        ti_thing_t * thing,
        const char * str,
        size_t n,
        ti_val_t ** val,
        ex_t * e)
{
    ti_raw_t * key = ti_str_create(str, n);
    if (!key)
    {
        ex_set_mem(e);
        return e->nr;
    }

    assert (ti_thing_is_dict(thing));

    if (ti_val_make_assignable(val, thing, key, e))
        return e->nr;

    if (thing_i__item_set_e(thing, key, *val, e))
    {
        assert (e->nr);
        ti_val_unsafe_drop((ti_val_t *) key);
        return e->nr;
    }

    ti_incref(*val);

    witem->key = key;
    witem->val = val;

    return 0;
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

    if (ti_thing_is_dict(thing)
            ? thing_i__item_set_e(thing, (ti_raw_t *) name, *val, e)
            : thing_p__prop_set_e(thing, name, *val, e))
    {
        assert (e->nr);
        ti_name_unsafe_drop(name);
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
    if (!field)
        return e->nr;

    vaddr = (ti_val_t **) vec_get_addr(thing->items.vec, field->idx);
    if (thing__val_locked(thing, field->name, *vaddr, e) ||
        ti_field_make_assignable(field, val, thing, e))
        return e->nr;

    ti_val_unsafe_gc_drop(*vaddr);
    *vaddr = *val;

    ti_incref(*val);

    wprop->name = field->name;
    wprop->val = val;

    return 0;
}

/* Deletes a property */
void ti_thing_o_del(ti_thing_t * thing, const char * str, size_t n)
{
    if (ti_thing_is_dict(thing))
    {
        ti_item_t * item = smap_popn(thing->items.smap, str, n);
        ti_item_destroy(item);
    }
    else
    {
        uint32_t idx = 0;
        ti_name_t * name = ti_names_weak_get_strn(str, n);
        if (!name)
            return;

        for (vec_each(thing->items.vec, ti_prop_t, prop), ++idx)
        {
            if (prop->name == name)
            {
                ti_prop_destroy(vec_swap_remove(thing->items.vec, idx));
                return;
            }
        }
    }
}

/* Returns the removed property or NULL in case of an error */
ti_item_t * ti_thing_o_del_e(ti_thing_t * thing, ti_raw_t * rname, ex_t * e)
{
    if (ti_thing_is_dict(thing))
    {
        ti_item_t * item = smap_popn(
                thing->items.smap,
                (const char *) rname->data,
                rname->n);
        if (!item)
            goto not_found;

        return thing__item_val_locked(thing, item->key, item->val, e)
                ? NULL
                : item;
    }
    else
    {
        uint32_t i = 0;
        ti_name_t * name = ti_names_weak_from_raw(rname);

        if (name)
        {
            for (vec_each(thing->items.vec, ti_prop_t, prop), ++i)
            {
                if (prop->name == name)
                    return thing__val_locked(thing, prop->name, prop->val, e)
                        ? NULL
                        : vec_swap_remove(thing->items.vec, i);
            }
        }
    }

not_found:
    ti_thing_o_set_not_found(thing, rname, e);
    return NULL;
}

static _Bool thing_p__get_by_name(
        ti_wprop_t * wprop,
        ti_thing_t * thing,
        ti_name_t * name)
{
    for (vec_each(thing->items.vec, ti_prop_t, prop))
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
    ti_type_t * type = ti_thing_type(thing);
    ti_field_t * field;
    ti_method_t * method;

    if ((field = ti_field_by_name(type, name)))
    {
        wprop->name = name;
        wprop->val = (ti_val_t **) vec_get_addr(thing->items.vec, field->idx);
        return true;
    }

    if ((method = ti_method_by_name(type, name)))
    {
        wprop->name = name;
        wprop->val = (ti_val_t **) (&method->closure);
        return true;
    }

    return false;
}

static _Bool thing_i__get_by_key(
        ti_witem_t * witem,
        ti_thing_t * thing,
        ti_raw_t * key)
{
    ti_item_t * item = smap_getn(
            thing->items.smap,
            (const char *) key->data,
            key->n);
    if (!item)
        return false;

    witem->key = key;
    witem->val = &item->val;
    return true;
}

_Bool ti_thing_get_by_raw(ti_witem_t * witem, ti_thing_t * thing, ti_raw_t * r)
{
    ti_name_t * name;

    if (ti_thing_is_dict(thing))
        return thing_i__get_by_key(witem, thing, r);

    name = r->tp == TI_VAL_NAME
            ? (ti_name_t *) r
            : ti_names_weak_from_raw(r);
    return name && (ti_thing_is_object(thing)
            ? thing_p__get_by_name((ti_wprop_t *) witem, thing, name)
            : thing_t__get_by_name((ti_wprop_t *) witem, thing, name));
}

int ti_thing_get_by_raw_e(
        ti_witem_t * witem,
        ti_thing_t * thing,
        ti_raw_t * r,
        ex_t * e)
{
    ti_name_t * name;

    if (ti_thing_is_dict(thing))
    {
        if (!thing_i__get_by_key(witem, thing, r))
            ti_thing_o_set_not_found(thing, r, e);
        return e->nr;
    }

    name = ti_names_weak_from_raw(r);

    if (ti_thing_is_object(thing))
    {
        if (!name || !thing_p__get_by_name((ti_wprop_t *) witem, thing, name))
            ti_thing_o_set_not_found(thing, r, e);
        return e->nr;
    }

    if (!name || !thing_t__get_by_name((ti_wprop_t *) witem, thing, name))
        ti_thing_t_set_not_found(thing, name, r, e);

    return e->nr;
}

static inline int thing__gen_id_i_cb(ti_item_t * item, void * UNUSED(arg))
{
    return ti_val_gen_ids(item->val);
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
        if (ti_thing_is_dict(thing))
            return smap_values(
                    thing->items.smap,
                    (smap_val_cb) thing__gen_id_i_cb,
                    NULL);

        for (vec_each(thing->items.vec, ti_prop_t, prop))
            if (ti_val_gen_ids(prop->val))
                return -1;
        return 0;
    }

    /* type */
    for (vec_each(thing->items.vec, ti_val_t, val))
        if (ti_val_gen_ids(val))
            return -1;
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

        switch((*val)->tp)
        {
        case TI_VAL_ARR:
            ((ti_varr_t *) *val)->key_ = name;
            break;
        case TI_VAL_SET:
            ((ti_vset_t *) *val)->key_ = name;
            break;
        }

        ti_incref(name);
        *val = (ti_val_t *) prop;
    }
    thing->type_id = TI_SPEC_OBJECT;
}

typedef struct
{
    int options;
    msgpack_packer * pk;
} thing__pk_cb_t;

static int thing__pk_cb(ti_item_t * item, thing__pk_cb_t * w)
{
    return (
        mp_pack_strn(w->pk, item->key->data, item->key->n) ||
        ti_val_to_pk(item->val, w->pk, w->options)
    );
}

int ti_thing__to_pk(ti_thing_t * thing, msgpack_packer * pk, int options)
{
    assert (options);  /* should be either positive or negative, not 0 */

    if (options > 0)
    {
        /*
         * Only when packing for a client the result size is checked;
         * The correct error is not set here, but instead the size should be
         * checked again to set either a `memory` or `too_much_data` error.
         */
        if (((msgpack_sbuffer *) pk->data)->size > ti.cfg->result_size_limit)
            return -1;

        --options;
    }

    if (msgpack_pack_map(pk, (!!thing->id) + ti_thing_n(thing)))
        return -1;

    if (thing->id && (
            mp_pack_strn(pk, TI_KIND_S_THING, 1) ||
            msgpack_pack_uint64(pk, thing->id)
    )) return -1;

    thing->flags |= TI_VFLAG_LOCK;

    if (ti_thing_is_object(thing))
    {
        if (ti_thing_is_dict(thing))
        {
            thing__pk_cb_t w = {
                    .options = options,
                    .pk = pk,
            };
            if (smap_values(
                    thing->items.smap,
                    (smap_val_cb) thing__pk_cb,
                    &w))
                goto fail;
        }
        else
        {
            for (vec_each(thing->items.vec, ti_prop_t, prop))
            {
                if (mp_pack_strn(pk, prop->name->str, prop->name->n) ||
                    ti_val_to_pk(prop->val, pk, options)
                ) goto fail;
            }
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
        msgpack_pack_array(pk, ti_thing_n(thing)))
        return -1;

    for (vec_each(thing->items.vec, ti_val_t, val))
        if (ti_val_to_pk(val, pk, options))
            return -1;

    return 0;
}

ti_val_t * ti_thing_val_by_strn(ti_thing_t * thing, const char * str, size_t n)
{
    if (ti_thing_is_dict(thing))
    {
        ti_item_t * item = smap_getn(thing->items.smap, str, n);
        return item ? item->val : NULL;
    }
    else
    {
        ti_name_t * name = ti_names_weak_get_strn(str, n);
        if (!name)
            return NULL;

        return ti_thing_is_object(thing)
                ? ti_thing_p_val_weak_get(thing, name)
                : ti_thing_t_val_weak_get(thing, name);
    }
}

_Bool ti__thing_has_watchers_(ti_thing_t * thing)
{
    assert (thing->watchers);
    for (vec_each(thing->watchers, ti_watch_t, watch))
        if (watch->stream && (~watch->stream->flags & TI_STREAM_FLAG_CLOSED))
            return true;
    return false;
}

static inline _Bool thing__val_equals(ti_val_t * a, ti_val_t * b, uint8_t deep)
{
    return ti_val_is_thing(a)
            ? ti_thing_equals((ti_thing_t *) a, b, deep)
            : ti_opr_eq(a, b);
}

typedef struct
{
    uint8_t deep;
    ti_thing_t * other;
} thing__equals_t;

static int thing__equals_i_i_cb(ti_item_t * item, thing__equals_t * w)
{
    ti_item_t * i = smap_getn(
            w->other->items.smap,
            (const char *) item->key->data,
            item->key->n);
    return !i || !thing__val_equals(item->val, i->val, w->deep);
}

static int thing__equals_i_p_cb(ti_item_t * item, thing__equals_t * w)
{
    if (ti_raw_is_name(item->key))
    {
        ti_val_t * b = ti_thing_p_val_weak_get(
                w->other,
                (ti_name_t *) item->key);
        return !b || !thing__val_equals(item->val, b, w->deep);
    }
    return 1;
}

static int thing__equals_i_t_cb(ti_item_t * item, thing__equals_t * w)
{
    if (ti_raw_is_name(item->key))
    {
        ti_val_t * b = ti_thing_t_val_weak_get(
                w->other,
                (ti_name_t *) item->key);
        return !b || !thing__val_equals(item->val, b, w->deep);
    }
    return 1;
}

_Bool ti_thing_equals(ti_thing_t * thing, ti_val_t * otherv, uint8_t deep)
{
    ti_thing_t * other = (ti_thing_t *) otherv;
    if (!ti_val_is_thing(otherv))
        return false;
    if (thing == other || !deep)
        return true;
    if (ti_thing_n(thing) != ti_thing_n(other))
        return false;

    --deep;

    if (ti_thing_is_object(thing))
    {
        if (ti_thing_is_dict(thing))
        {
            thing__equals_t w = {
                    .deep = deep,
                    .other = other,
            };
            smap_val_cb cb = ti_thing_is_object(other)
                    ? ti_thing_is_dict(other)
                    ? (smap_val_cb) thing__equals_i_i_cb
                    : (smap_val_cb) thing__equals_i_p_cb
                    : (smap_val_cb) thing__equals_i_t_cb;
            return !smap_values(thing->items.smap, cb, &w);
        }
        for (vec_each(thing->items.vec, ti_prop_t, prop))
        {
            ti_val_t * b = ti_thing_val_weak_by_name(other, prop->name);
            if (!b || !thing__val_equals(prop->val, b, deep))
                return false;
        }
    }
    else if (thing->type_id == other->type_id)
    {
        size_t idx = 0;
        for (vec_each(thing->items.vec, ti_val_t, a), ++idx)
        {
            ti_val_t * b = VEC_get(other->items.vec, idx);
            if (!thing__val_equals(a, b, deep))
                return false;
        }
    }
    else
    {
        ti_name_t * name;
        ti_val_t * a;
        for (thing_t_each(thing, name, a))
        {
            ti_val_t * b = ti_thing_val_weak_by_name(other, name);
            if (!b || !thing__val_equals(a, b, deep))
                return false;
        }
    }
    return true;
}

int ti_thing_init_gc(void)
{
    thing__gc_swp = vec_new(8);
    ti_thing_gc_vec = vec_new(8);

    if (!thing__gc_swp || !ti_thing_gc_vec)
    {
        free(thing__gc_swp);
        free(ti_thing_gc_vec);
        return -1;
    }
    return 0;
}

void ti_thing_resize_gc(void)
{
    assert (thing__gc_swp->n == 0);
    assert (ti_thing_gc_vec->n == 0);

    if (thing__gc_swp->sz > 2047)
    {
        (void) vec_resize(&thing__gc_swp, 2047);
    }
    else if (thing__gc_swp->sz > ti_thing_gc_vec->sz)
    {
        (void) vec_resize(&thing__gc_swp, ti_thing_gc_vec->sz);
        return;
    }

    if (ti_thing_gc_vec->sz > 2047)
    {
        (void) vec_resize(&ti_thing_gc_vec, 2047);
    }
    else if (ti_thing_gc_vec->sz > thing__gc_swp->sz)
    {
        (void) vec_resize(&ti_thing_gc_vec, thing__gc_swp->sz);
        return;
    }
}

void ti_thing_destroy_gc(void)
{
    free(thing__gc_swp);
    free(ti_thing_gc_vec);
}

void ti_thing_clean_gc(void)
{
    while (ti_thing_gc_vec->n)
    {
        vec_t * tmp = ti_thing_gc_vec;
        ti_thing_gc_vec = thing__gc_swp;
        thing__gc_swp = tmp;

        for (vec_each(tmp, ti_thing_t, thing))
        {
            if (thing->id)
                thing->flags |= TI_THING_FLAG_SWEEP;
            else
                ti_thing_clear(thing);

            ti_val_unsafe_drop((ti_val_t *) thing);
        }
        vec_clear(tmp);
    }
}
