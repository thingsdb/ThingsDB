/*
 * thing.c
 */
#include <assert.h>
#include <doc.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/changes.inline.h>
#include <ti/enums.h>
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
#include <ti/spec.inline.h>
#include <ti/task.h>
#include <ti/thing.h>
#include <ti/thing.inline.h>
#include <ti/types.h>
#include <ti/val.h>
#include <ti/val.inline.h>
#include <ti/vp.t.h>
#include <ti/watch.h>
#include <util/logger.h>
#include <util/mpack.h>

static vec_t * thing__gc_swp;
vec_t * ti_thing_gc_vec;


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
    thing->via.spec = TI_SPEC_ANY;

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
    thing->via.spec = TI_SPEC_ANY;

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
    thing->via.type = type;

    if (!thing->items.vec)
    {
        ti_thing_destroy(thing);
        return NULL;
    }
    return thing;
}

typedef struct
{
    ti_field_t * field;
    ti_thing_t * thing;
} thing__clear_rel_t;

int thing__clear_rel_cb(ti_thing_t * other, thing__clear_rel_t * w)
{
    w->field->condition.rel->del_cb(w->field, other, w->thing);
    return 0;
}

void ti_thing_cancel(ti_thing_t * thing)
{
    if (ti_thing_is_instance(thing))
    {
        /* Remove SWEEP flag to prevent garbage collection */
        thing->flags &= ~TI_THING_FLAG_SWEEP;

        for (vec_each(thing->via.type->fields, ti_field_t, field))
        {
            if (!field->condition.none)
                continue;

            if (field->spec == TI_SPEC_SET)
            {
                ti_field_t * ofield = field->condition.rel->field;
                ti_vset_t * vset = vec_get(thing->items.vec, field->idx);
                if (!vset)
                    continue;
                thing__clear_rel_t w = {
                        .field = ofield,
                        .thing = thing
                };
                (void) imap_walk(vset->imap, (imap_cb) thing__clear_rel_cb, &w);
            }
            else if ((field->spec & TI_SPEC_MASK_NILLABLE) < TI_SPEC_ANY)
            {
                ti_field_t * ofield = field->condition.rel->field;
                ti_thing_t * other = vec_get(thing->items.vec, field->idx);

                if (!other || ti_val_is_nil((ti_val_t *) other))
                    continue;

                ofield->condition.rel->del_cb(ofield, other, thing);
                field->condition.rel->del_cb(field, thing, NULL);
            }
        }
    }
    ti_thing_destroy(thing);
}

void ti_thing_destroy(ti_thing_t * thing)
{
    assert (thing);
    if (thing->id)
    {
        if (ti_changes_cache_dropped_thing(thing))
            return;

        (void) imap_pop(thing->collection->things, thing->id);
        /*
         * It is not possible that the thing exist in garbage collection
         * since the garbage collector hold a reference to the thing and
         * will therefore never destroy.
         */
    }

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

    free(thing);
}

void ti_thing_clear(ti_thing_t * thing)
{
    if (ti_thing_is_object(thing))
    {
        if (ti_thing_is_dict(thing))
            smap_clear(thing->items.smap, (smap_destroy_cb) ti_item_destroy);
        else
            vec_clear_cb(thing->items.vec, (vec_destroy_cb) ti_prop_destroy);
    }
    else
    {
        vec_clear_cb(thing->items.vec, (vec_destroy_cb) ti_val_unsafe_gc_drop);

        /* convert to a simple object since the thing is not type
         * compliant anymore */
        thing->type_id = TI_SPEC_OBJECT;
        thing->via.spec = TI_SPEC_ANY;
    }
}

void ti_thing_o_items_destroy(ti_thing_t * thing)
{
    assert(ti_thing_is_object(thing));

    if (ti_thing_is_dict(thing))
    {
        smap_destroy(
                thing->items.smap,
                (smap_destroy_cb) ti_prop_unsafe_vdestroy);
    }
    else
    {
        vec_destroy(
                thing->items.vec,
                (vec_destroy_cb) ti_prop_unsafe_vdestroy);
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

static int thing__i_to_p_cb(ti_item_t * item, thing__to_stric_t * w)
{
    if (ti_raw_is_name(item->key))
    {
        VEC_push(w->vec, item);
        return 0;
    }
    *w->incompatible = item->key;
    return -1;
}

int ti_thing_i_to_p(ti_thing_t * thing, ti_raw_t ** incompatible)
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
            (smap_val_cb) thing__i_to_p_cb,
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
            ex_set(e, EX_TYPE_ERROR,
                    "property names must be of type `"TI_VAL_STR_S"`");
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
    ti_thing_t * thing = ti_thing_o_create(0, sz, vup->collection);
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
            if (ti_val_tlocked(p->val, thing, p->name, e))
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
        if (ti_val_tlocked(item->val, thing, (ti_name_t *) item->key, e))
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
        return free(prop), NULL;

    return prop;
}

/*
 * Does not increment the `name` and `val` reference counters.
 */
ti_item_t* ti_thing_i_item_set(
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
        /*
         * The key is equal, so we do not have to anything with the key.
         */

        /* bug #291 */
        ti_val_unsafe_drop((ti_val_t *) key);

        ti_val_unsafe_gc_drop(item->val);
        item->val = val;
        return item;
    }

    item = ti_item_create(key, val);
    if (smap_addn(thing->items.smap, (const char *) key->data, key->n, item))
        return free(item), NULL;

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
        return ex_set_mem(e), e->nr;

    if (ti_val_make_assignable(val, thing, key, e))
        return e->nr;

    if (thing_i__item_set_e(thing, key, *val, e))
    {
        ti_val_unsafe_drop((ti_val_t *) key);
        return e->nr;
    }

    ti_incref(*val);

    witem->key = key;
    witem->val = val;

    return 0;
}

int ti_thing_o_set_val_from_strn(
        ti_witem_t * witem,
        ti_thing_t * thing,
        const char * str,
        size_t n,
        ti_val_t ** val,
        ex_t * e)
{
    if (ti_name_is_valid_strn(str, n))
        /* Create a name when the key is a valid name, this is required since
         * some logic, for example in `do.c` checks if a name exists, and from
         * that result might decide a property exists or not.
         */
        return ti_thing_o_set_val_from_valid_strn(
                (ti_wprop_t *) witem,
                thing, str, n, val, e);

    if (!ti_val_is_spec(*val, thing->via.spec))
    {
        ex_set(e, EX_TYPE_ERROR, "restriction mismatch");
        return e->nr;
    }

    if (!strx_is_utf8n(str, n))
    {
        ex_set(e, EX_VALUE_ERROR, "properties must have valid UTF-8 encoding");
        return e->nr;
    }

    if (ti_is_reserved_key_strn(str, n))
    {
        ex_set(e, EX_VALUE_ERROR, "property `%c` is reserved"DOC_PROPERTIES,
                *str);
        return e->nr;
    }

    if (!ti_thing_is_dict(thing) && ti_thing_to_dict(thing))
        return ex_set_mem(e), e->nr;

    return ti_thing_i_set_val_from_strn(witem, thing, str, n, val, e);
}

/*
 * Return 0 if successful; This function makes a given `value` assignable so
 * it should not be used within a task.
 */
int ti_thing_o_set_val_from_valid_strn(
        ti_wprop_t * wprop,
        ti_thing_t * thing,
        const char * str,
        size_t n,
        ti_val_t ** val,
        ex_t * e)
{
    ti_name_t * name;

    if (!ti_val_is_spec(*val, thing->via.spec))
    {
        ex_set(e, EX_TYPE_ERROR, "restriction mismatch");
        return e->nr;
    }

    name = ti_names_get(str, n);
    if (!name)
        return ex_set_mem(e), e->nr;

    if (ti_val_make_assignable(val, thing, name, e))
    {
        ti_name_unsafe_drop(name);
        return e->nr;
    }

    if (ti_thing_is_dict(thing)
            ? thing_i__item_set_e(thing, (ti_raw_t *) name, *val, e)
            : thing_p__prop_set_e(thing, name, *val, e))
    {
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
 * it should not be used within a task.
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
    ti_field_t * field = ti_field_by_strn_e(thing->via.type, str, n, e);
    if (!field)
        return e->nr;

    vaddr = (ti_val_t **) vec_get_addr(thing->items.vec, field->idx);
    if (ti_val_tlocked(*vaddr, thing, field->name, e) ||
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

        if (ti_val_tlocked(item->val, thing, (ti_name_t *) item->key, e))
        {
            if (smap_addn(
                    thing->items.smap,
                    (const char *) rname->data,
                    rname->n,
                    item))
                log_critical("failed to restore item");
            return NULL;
        }
        return item;
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
                    return ti_val_tlocked(prop->val, thing, prop->name, e)
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
    ti_field_t * field;
    ti_method_t * method;

    if ((field = ti_field_by_name(thing->via.type, name)))
    {
        wprop->name = name;
        wprop->val = (ti_val_t **) vec_get_addr(thing->items.vec, field->idx);
        return true;
    }

    if ((method = ti_method_by_name(thing->via.type, name)))
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

    thing->id = ti_next_free_id();
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
    thing->via.spec = TI_SPEC_ANY;  /* fixes bug #277 */
}

typedef struct
{
    int deep;
    int flags;
    ti_vp_t * vp;
} thing__pk_cb_t;

static inline int thing__client_pk_cb(ti_item_t * item, thing__pk_cb_t * w)
{
    return -(
        mp_pack_strn(&w->vp->pk, item->key->data, item->key->n) ||
        ti_val_to_client_pk(item->val, w->vp, w->deep, w->flags)
    );
}

int ti_thing__to_client_pk(
        ti_thing_t * thing,
        ti_vp_t * vp,
        int deep,
        int flags)
{
    register uint8_t with_id = thing->id && (~flags & TI_FLAGS_NO_IDS);
    /*
     * Only when packing for a client the result size is checked;
     * The correct error is not set here, but instead the size should be
     * checked again to set either a `memory` or `too_much_data` error.
     */
    if (((msgpack_sbuffer *) vp->pk.data)->size >
                ti.cfg->result_size_limit)
        return -1;

    --deep;

    thing->flags |= TI_VFLAG_LOCK;

    if (ti_thing_is_object(thing))
    {
        if (msgpack_pack_map(&vp->pk, with_id + ti_thing_n(thing)))
            return -1;

        if (with_id && (
                mp_pack_strn(&vp->pk, TI_KIND_S_THING, 1) ||
                msgpack_pack_uint64(&vp->pk, thing->id)
        )) goto fail;

        if (ti_thing_is_dict(thing))
        {
            thing__pk_cb_t w = {
                    .deep = deep,
                    .flags = flags,
                    .vp = vp,
            };
            if (smap_values(
                    thing->items.smap,
                    (smap_val_cb) thing__client_pk_cb,
                    &w))
                goto fail;
        }
        else
        {
            for (vec_each(thing->items.vec, ti_prop_t, prop))
            {
                if (mp_pack_strn(&vp->pk, prop->name->str, prop->name->n) ||
                    ti_val_to_client_pk(prop->val, vp, deep, flags)
                ) goto fail;
            }
        }
    }
    else
    {
        ti_name_t * name = thing->via.type->idname;
        ti_val_t * val;

        with_id = with_id && !(thing->via.type->flags & TI_TYPE_FLAG_HIDE_ID);

        if (msgpack_pack_map(&vp->pk, with_id + ti_thing_n(thing)))
            return -1;

        if (with_id && ((name
                    ? mp_pack_strn(&vp->pk, name->str, name->n)
                    : mp_pack_strn(&vp->pk, TI_KIND_S_THING, 1)) ||
                msgpack_pack_uint64(&vp->pk, thing->id)
        )) goto fail;

        for (thing_t_each(thing, name, val))
        {
            if (mp_pack_strn(&vp->pk, name->str, name->n) ||
                ti_val_to_client_pk(val, vp, deep, flags)
            ) goto fail;
        }
    }

    thing->flags &= ~TI_VFLAG_LOCK;
    return 0;
fail:
    thing->flags &= ~TI_VFLAG_LOCK;
    return -1;
}

static inline int thing__store_pk_cb(ti_item_t * item, msgpack_packer * pk)
{
    return -(
        mp_pack_strn(pk, item->key->data, item->key->n) ||
        ti_val_to_store_pk(item->val, pk)
    );
}

int ti_thing_o_to_pk(ti_thing_t * thing, msgpack_packer * pk)
{
    assert (ti_thing_is_object(thing));
    assert (thing->id);   /* no need to check, options < 0 must have id */

    if (msgpack_pack_map(pk, 1) ||
        mp_pack_strn(pk, TI_KIND_S_OBJECT, 1) ||
        msgpack_pack_array(pk, 3) ||
        msgpack_pack_uint16(pk, thing->via.spec) ||
        msgpack_pack_uint64(pk, thing->id) ||
        msgpack_pack_map(pk, ti_thing_n(thing)))
        return -1;

    if (ti_thing_is_dict(thing))
        return smap_values(
                thing->items.smap,
                (smap_val_cb) thing__store_pk_cb,
                pk);

    for (vec_each(thing->items.vec, ti_prop_t, prop))
        if (mp_pack_strn(pk, prop->name->str, prop->name->n) ||
            ti_val_to_store_pk(prop->val, pk))
            return -1;

    return 0;
}

int ti_thing_t_to_pk(ti_thing_t * thing, msgpack_packer * pk)
{
    assert (!ti_thing_is_object(thing));
    assert (thing->id);   /* no need to check, options < 0 must have id */

    if (msgpack_pack_map(pk, 1) ||
        mp_pack_strn(pk, TI_KIND_S_INSTANCE, 1) ||
        msgpack_pack_array(pk, 3) ||
        msgpack_pack_uint16(pk, thing->type_id) ||
        msgpack_pack_uint64(pk, thing->id) ||
        msgpack_pack_array(pk, ti_thing_n(thing)))
        return -1;

    for (vec_each(thing->items.vec, ti_val_t, val))
        if (ti_val_to_store_pk(val, pk))
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

static int thing__copy_p(ti_thing_t ** taddr, uint8_t deep)
{
    ti_thing_t * thing = *taddr;
    ti_thing_t * other = ti_thing_o_create(
            0,
            ti_thing_n(thing),
            thing->collection);

    if (!other)
        return -1;

    for (vec_each(thing->items.vec, ti_prop_t, prop))
    {
        ti_prop_t * p = ti_prop_dup(prop);

        if (!p || ti_val_copy(&p->val, other, p->name, deep))
        {
            ti_prop_destroy(p);
            goto fail;
        }

        VEC_push(other->items.vec, p);
    }

    ti_val_unsafe_gc_drop((ti_val_t *) thing);
    *taddr = other;
    return 0;

fail:
    ti_val_unsafe_drop((ti_val_t *) other);
    return -1;
}

static int thing__dup_p(ti_thing_t ** taddr, uint8_t deep)
{
    ti_thing_t * thing = *taddr;
    ti_thing_t * other = ti_thing_o_create(
            0,
            ti_thing_n(thing),
            thing->collection);

    if (!other)
        return -1;

    other->via.spec = thing->via.spec;

    for (vec_each(thing->items.vec, ti_prop_t, prop))
    {
        ti_prop_t * p = ti_prop_dup(prop);

        if (!p || ti_val_dup(&p->val, other, p->name, deep))
        {
            ti_prop_destroy(p);
            goto fail;
        }

        VEC_push(other->items.vec, p);
    }

    ti_val_unsafe_gc_drop((ti_val_t *) thing);
    *taddr = other;
    return 0;

fail:
    ti_val_unsafe_drop((ti_val_t *) other);
    return -1;
}

typedef struct
{
    ti_thing_t * other;
    uint8_t deep;
} thing__wcd_t;

static int thing__copy_cb(ti_item_t * item, thing__wcd_t * w)
{
    ti_item_t * i = ti_item_dup(item);
    if (!i ||
        ti_val_copy(&i->val, w->other, i->key, w->deep) ||
        smap_addn(
                w->other->items.smap,
                (const char *) i->key->data,
                i->key->n,
                i))
    {
        ti_item_destroy(i);
        return -1;
    }

    return 0;
}

static int thing__dup_cb(ti_item_t * item, thing__wcd_t * w)
{
    ti_item_t * i = ti_item_dup(item);
    if (!i ||
        ti_val_dup(&i->val, w->other, i->key, w->deep) ||
        smap_addn(
                w->other->items.smap,
                (const char *) i->key->data,
                i->key->n,
                i))
    {
        ti_item_destroy(i);
        return -1;
    }

    return 0;
}


static int thing__copy_i(ti_thing_t ** taddr, uint8_t deep)
{
    ti_thing_t * thing = *taddr;
    ti_thing_t * other = ti_thing_i_create(0, thing->collection);
    thing__wcd_t w = {
            .other = other,
            .deep = deep,
    };

    if (!other)
        return -1;

    if (smap_values(thing->items.smap, (smap_val_cb) thing__copy_cb, &w))
        goto fail;

    ti_val_unsafe_gc_drop((ti_val_t *) thing);
    *taddr = other;
    return 0;

fail:
    ti_val_unsafe_drop((ti_val_t *) other);
    return -1;
}

static int thing__dup_i(ti_thing_t ** taddr, uint8_t deep)
{
    ti_thing_t * thing = *taddr;
    ti_thing_t * other = ti_thing_i_create(0, thing->collection);
    thing__wcd_t w = {
            .other = other,
            .deep = deep,
    };

    if (!other)
        return -1;

    other->via.spec = thing->via.spec;

    if (smap_values(thing->items.smap, (smap_val_cb) thing__dup_cb, &w))
        goto fail;

    ti_val_unsafe_gc_drop((ti_val_t *) thing);
    *taddr = other;
    return 0;

fail:
    ti_val_unsafe_drop((ti_val_t *) other);
    return -1;
}

static int thing__copy_t(ti_thing_t ** taddr, uint8_t deep)
{
    ti_name_t * name;
    ti_val_t * val;
    ti_thing_t * thing = *taddr;
    ti_thing_t * other = ti_thing_o_create(
            0,
            ti_thing_n(thing),
            thing->collection);


    if (!other)
        return -1;

    for(thing_t_each(thing, name, val))
    {
        ti_prop_t * p = ti_prop_create(name, val);
        if (!p)
            goto fail;

        ti_incref(name);
        ti_incref(val);

        if (ti_val_copy(&p->val, other, p->name, deep))
        {
            ti_prop_destroy(p);
            goto fail;
        }

        VEC_push(other->items.vec, p);
    }

    ti_val_unsafe_gc_drop((ti_val_t *) thing);
    *taddr = other;
    return 0;

fail:
    ti_val_unsafe_drop((ti_val_t *) other);
    return -1;
}

static int thing__dup_t(ti_thing_t ** taddr, uint8_t deep)
{
    ti_val_t * val;
    ti_thing_t * thing = *taddr;
    ti_type_t * type = thing->via.type;
    ti_thing_t * other = ti_thing_t_create(0, type, thing->collection);
    if (!other)
        return -1;

    for (vec_each(type->fields, ti_field_t, field))
    {
        val = VEC_get(thing->items.vec, field->idx);
        ti_incref(val);
        if (ti_val_dup(&val, other, field, deep))
        {
            ti_decref(val);
            goto fail;
        }
        VEC_push(other->items.vec, val);
    }

    ti_val_unsafe_gc_drop((ti_val_t *) thing);
    *taddr = other;
    return 0;

fail:
    ti_val_unsafe_drop((ti_val_t *) other);
    return -1;
}

int ti_thing_copy(ti_thing_t ** thing, uint8_t deep)
{
    assert (deep);
    return deep--
            ? ti_thing_is_object(*thing)
            ? ti_thing_is_dict(*thing)
            ? thing__copy_i(thing, deep)
            : thing__copy_p(thing, deep)
            : thing__copy_t(thing, deep)
            : 0;
}

int ti_thing_dup(ti_thing_t ** thing, uint8_t deep)
{
    assert (deep);
    return deep--
            ? ti_thing_is_object(*thing)
            ? ti_thing_is_dict(*thing)
            ? thing__dup_i(thing, deep)
            : thing__dup_p(thing, deep)
            : thing__dup_t(thing, deep)
            : 0;
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
    if (ti.futures_count)
        return;

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

static int thing__assign_set_o(
        ti_thing_t * thing,
        ti_raw_t * key,
        ti_val_t * val,
        ti_task_t * task,
        ex_t * e)
{
    assert (ti_val_is_spec(val, thing->via.spec));
    ti_incref(val);

    /*
     * Closures are already unbound so the only possible errors are
     * critical.
     */
    if (ti_val_make_assignable(&val, thing, key, e) ||
        ti_thing_o_set(thing, key, val) ||
        (task && ti_task_add_set(task, key, val)))
        goto failed;

    ti_incref(key);
    return e->nr;

failed:
    ti_val_unsafe_gc_drop(val);
    if (e->nr == 0)
        ex_set_mem(e);

    return e->nr;
}

typedef struct
{
    ti_thing_t * thing;
    ti_thing_t * tsrc;
    ti_task_t * task;
    ex_t * e;
} thing__assign_walk_i_t;

static int thing__assign_walk_i(ti_item_t * item, thing__assign_walk_i_t * w)
{
    return thing__assign_set_o(
            w->thing,
            item->key,
            item->val,
            w->task,
            w->e);
}

static int thing__assign_restr_i(ti_item_t * item, thing__assign_walk_i_t * w)
{
    return !ti_val_is_spec(item->val, w->thing->via.spec);
}

int ti_thing_assign(
        ti_thing_t * thing,
        ti_thing_t * tsrc,
        ti_task_t * task,
        ex_t * e)
{
    if (ti_thing_is_object(thing))
    {
        if (ti_thing_is_object(tsrc))
        {
            if (ti_thing_is_dict(tsrc))
            {
                thing__assign_walk_i_t w = {
                        .thing = thing,
                        .tsrc = tsrc,
                        .task = task,
                        .e = e,
                };

                if (thing->via.spec != TI_SPEC_ANY && smap_values(
                        tsrc->items.smap,
                        (smap_val_cb) thing__assign_restr_i,
                        &w))
                    goto mismatch;

                if (smap_values(
                        tsrc->items.smap,
                        (smap_val_cb) thing__assign_walk_i,
                        &w))
                    return e->nr;
            }
            else
            {
                if (thing->via.spec != TI_SPEC_ANY)
                    for(vec_each(tsrc->items.vec, ti_prop_t, p))
                        if (!ti_val_is_spec(p->val, thing->via.spec))
                            goto mismatch;

                for(vec_each(tsrc->items.vec, ti_prop_t, p))
                    if (thing__assign_set_o(
                            thing,
                            (ti_raw_t *) p->name,
                            p->val,
                            task,
                            e))
                        return e->nr;
            }
        }
        else
        {
            ti_name_t * name;
            ti_val_t * val;

            if (thing->via.spec != TI_SPEC_ANY)
                for(thing_t_each(tsrc, name, val))
                    if (!ti_val_is_spec(val, thing->via.spec))
                        goto mismatch;

            for(thing_t_each(tsrc, name, val))
                if (thing__assign_set_o(
                        thing,
                        (ti_raw_t *) name,
                        val,
                        task,
                        e))
                    return e->nr;
        }
    }
    else
    {
        vec_t * vec = NULL;
        ti_type_t * type = thing->via.type;

        if (ti_thing_is_object(tsrc))
        {
            if (ti_thing_is_dict(tsrc))
            {
                ti_raw_t * incompatible;
                if (ti_thing_i_to_p(tsrc, &incompatible))
                {
                    if (incompatible)
                        ex_set(e, EX_LOOKUP_ERROR,
                                "type `%s` has no property `%s`",
                                type->name,
                                ti_raw_as_printable_str(incompatible));
                    else
                        ex_set_mem(e);

                    return e->nr;
                }
            }
            vec = vec_new(ti_thing_n(tsrc));
            if (!vec)
            {
                ex_set_mem(e);
                return e->nr;
            }

            for(vec_each(tsrc->items.vec, ti_prop_t, p))
            {
                ti_val_t * val = p->val;
                ti_field_t * field = ti_field_by_name(type, p->name);
                if (!field)
                {
                    ex_set(e, EX_LOOKUP_ERROR,
                            "type `%s` has no property `%s`",
                            type->name, p->name->str);
                    goto fail;
                }

                if (ti_field_has_relation(field))
                {
                    ex_set(e, EX_TYPE_ERROR,
                            "function `assign` is not able to set "
                            "`%s` because a relation for this "
                            "property is configured"DOC_THING_ASSIGN,
                            p->name->str);
                    goto fail;
                }

                ti_incref(val);
                if (ti_field_make_assignable(field, &val, thing, e))
                {
                    ti_val_unsafe_gc_drop(val);
                    goto fail;
                }
                VEC_push(vec, val);
            }

            for(vec_each_rev(tsrc->items.vec, ti_prop_t, p))
            {
                ti_field_t * field = ti_field_by_name(type, p->name);
                ti_val_t * val = VEC_pop(vec);
                ti_thing_t_prop_set(thing, field, val);
                if (task && ti_task_add_set(
                        task,
                        (ti_raw_t *) field->name,
                        val))
                    goto fail;
            }
        }
        else
        {
            ti_type_t * f_type = tsrc->via.type;
            vec = NULL;
            if (f_type != type)
            {
                ex_set(e, EX_TYPE_ERROR,
                        "cannot assign properties to instance of type `%s` "
                        "from type `%s`"
                        DOC_THING_ASSIGN,
                        type->name,
                        f_type->name);
                return e->nr;
            }

            for (vec_each(type->fields, ti_field_t, field))
            {
                ti_val_t * val = VEC_get(tsrc->items.vec, field->idx);

                ti_incref(val);
                if (ti_field_make_assignable(field, &val, thing, e))
                {
                    ti_val_unsafe_gc_drop(val);
                    return e->nr;
                }
                ti_thing_t_prop_set(thing, field, val);
                if (task && ti_task_add_set(
                        task,
                        (ti_raw_t *) field->name,
                        val))
                    return e->nr;
            }
        }
fail:
        vec_destroy(vec, (vec_destroy_cb) ti_val_unsafe_drop);
    }
    return e->nr;

mismatch:
    ex_set(e, EX_TYPE_ERROR, "restriction mismatch");
    return e->nr;
}

typedef struct
{
    void * data;
    ti_thing_item_cb cb;
} thing__walk_i_t;

static int thing__walk_i(ti_item_t * item, thing__walk_i_t * w)
{
    return w->cb(item->key, item->val, w->data);
}

int ti_thing_walk(ti_thing_t * thing, ti_thing_item_cb cb, void * data)
{
    int rc;
    if (ti_thing_is_object(thing))
    {
        if (ti_thing_is_dict(thing))
        {
            thing__walk_i_t w = {
                    .data = data,
                    .cb = cb,
            };
            return smap_values(
                    thing->items.smap,
                    (smap_val_cb) thing__walk_i,
                    &w);
        }
        for (vec_each(thing->items.vec, ti_prop_t, p))
            if ((rc = cb((ti_raw_t *) p->name, p->val, data)))
                return rc;
        return 0;
    }
    else
    {
        ti_name_t * name;
        ti_val_t * val;
        for (thing_t_each(thing, name, val))
            if ((rc = cb((ti_raw_t *) name, val, data)))
                return rc;
        return 0;
    }
}
