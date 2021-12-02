
/*
 * ti/type.c
 */
#define _GNU_SOURCE
#include <assert.h>
#include <ctype.h>
#include <doc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/field.h>
#include <ti/gc.h>
#include <ti/mapping.h>
#include <ti/method.h>
#include <ti/names.h>
#include <ti/prop.h>
#include <ti/raw.inline.h>
#include <ti/task.h>
#include <ti/thing.inline.h>
#include <ti/type.h>
#include <ti/types.h>
#include <ti/types.inline.h>
#include <ti/val.inline.h>

static char * type__wrap_name(const char * name, size_t n)
{
    char * wname;
    return asprintf(&wname, "<%.*s>", (int) n, name) < 0
            ? NULL
            : wname;
}

ti_type_t * ti_type_create(
        ti_types_t * types,
        uint16_t type_id,
        uint8_t flags,
        const char * name,
        size_t name_n,
        uint64_t created_at,
        uint64_t modified_at)
{
    ti_type_t * type = malloc(sizeof(ti_type_t));
    if (!type)
        return NULL;

    type->refcount = 0;  /* only incremented when this type
                            is used by another type */
    type->type_id = type_id;
    type->flags = flags;
    type->name = strndup(name, name_n);
    type->wname = type__wrap_name(name, name_n);
    type->rname = ti_str_create(name, name_n);
    type->rwname = ti_str_from_str(type->wname);
    type->dependencies = vec_new(0);
    type->fields = vec_new(0);
    type->types = types;
    type->t_mappings = imap_create();
    type->created_at = created_at;
    type->modified_at = modified_at;
    type->methods = vec_new(0);

    if (!type->name || !type->wname || !type->dependencies || !type->fields ||
        !type->t_mappings || ti_types_add(types, type))
    {
        ti_type_destroy(type);
        return NULL;
    }

    return type;
}

/* used as a callback function and removes all cached type mappings */
static int type__map_cleanup(ti_type_t * t_haystack, ti_type_t * t_needle)
{
    vec_destroy(imap_pop(t_haystack->t_mappings, t_needle->type_id), free);
    return 0;
}

/* used as a callback function and destroys all type mappings */
static void type__map_free(void * vec)
{
    vec_destroy(vec, free);
}

void ti_type_map_cleanup(ti_type_t * type)
{
    (void) imap_walk(type->types->imap, (imap_cb) type__map_cleanup, type);
    imap_clear(type->t_mappings, type__map_free);
}

void ti_type_drop(ti_type_t * type)
{
    uintptr_t type_id;

    if (!type)
        return;

    type_id = (uintptr_t) type->type_id;
    type_id |= TI_TYPES_RM_FLAG;    /* prevents storing type_id 0 */

    for (vec_each(type->dependencies, ti_type_t, dep))
        --dep->refcount;

    (void) smap_add(type->types->removed, type->name, (void *) type_id);

    ti_type_map_cleanup(type);
    ti_types_del(type->types, type);
    ti_type_destroy(type);
}

static int type__del(ti_thing_t * thing, uint16_t * type_id)
{
    if (thing->type_id == *type_id)
        ti_thing_t_to_object(thing);
    else if (ti_thing_is_object(thing) &&
            (thing->via.spec & TI_SPEC_MASK_NILLABLE) == *type_id)
        thing->via.spec = TI_SPEC_ANY;
    return 0;
}

void ti_type_del(ti_type_t * type)
{
    assert (!type->refcount);

    ti_collection_t * collection = type->types->collection;
    uint16_t type_id = type->type_id;

    (void) imap_walk(collection->things, (imap_cb) type__del, &type_id);
    (void) ti_gc_walk(collection->gc, (queue_cb) type__del, &type_id);

    ti_type_drop(type);
}

void ti_type_destroy(ti_type_t * type)
{
    if (!type)
        return;

    vec_destroy(type->fields, (vec_destroy_cb) ti_field_destroy);
    vec_destroy(type->methods, (vec_destroy_cb) ti_method_destroy);
    imap_destroy(type->t_mappings, type__map_free);
    ti_val_drop((ti_val_t *) type->rname);
    ti_val_drop((ti_val_t *) type->rwname);
    free(type->dependencies);
    free(type->name);
    free(type->wname);
    free(type);
}

size_t ti_type_fields_approx_pack_sz(ti_type_t * type)
{
    size_t n = 0;
    for (vec_each(type->fields, ti_field_t, field))
        n += field->name->n + field->spec_raw->n + 10;
    return n;
}

int ti_type_rename(ti_type_t * type, ti_raw_t * nname)
{
    void * ptype;
    char * type_name;
    char * wtype_name;

    assert (nname->n <= TI_NAME_MAX);

    ptype = smap_popn(
            type->types->removed,
            (const char *) nname->data,
            nname->n);
    if (ptype)
    {
        /* swap new type name with the old type name */
        (void) smap_add(type->types->removed, type->name, ptype);
    }

    type_name = strndup((const char *) nname->data, nname->n);
    wtype_name = type__wrap_name((const char *) nname->data, nname->n);

    if (!type_name ||
        !wtype_name ||
        smap_addn(
                type->types->smap,
                (const char *) nname->data,
                nname->n,
                type))
    {
        free(type_name);
        free(wtype_name);
        return -1;
    }

    if (ti_types_rename_spec(type->types, type->type_id, type->rname, nname))
    {
        ti_panic("failed to rename all type");
        return -1;
    }

    (void) smap_pop(type->types->smap, type->name);

    free(type->name);
    free(type->wname);
    ti_val_unsafe_drop((ti_val_t *) type->rname);

    type->name = type_name;
    type->wname = wtype_name;
    type->rname = nname;

    ti_incref(nname);
    return 0;
}

int ti_type_add_method(
        ti_type_t * type,
        ti_name_t * name,
        ti_closure_t * closure,
        ex_t * e)
{
    ti_method_t * method = ti_method_create(name, closure);
    if (!method)
    {
        ex_set_mem(e);
        return e->nr;
    }

    if (vec_push(&type->methods, method))
    {
        ex_set_mem(e);
        ti_method_destroy(method);
    }

    return e->nr;
}

void ti_type_remove_method(ti_type_t * type, ti_name_t * name)
{
    size_t idx = 0;
    for (vec_each(type->methods, ti_method_t, m), ++idx)
    {
        if (m->name == name)
        {
            ti_method_destroy(vec_swap_remove(type->methods, idx));
            return;
        }
    }
}

static inline int type__assign(
        ti_type_t * type,
        ti_name_t * name,
        ti_val_t * val,
        ex_t * e)
{
    if (ti_val_is_str(val))
    {
        (void) ti_field_create(name, (ti_raw_t *) val, type, e);
        return e->nr;
    }

    if (ti_val_is_closure(val))
        return ti_type_add_method(type, name, (ti_closure_t *) val, e);

    ex_set(e, EX_TYPE_ERROR,
            "expecting a method of type `"TI_VAL_CLOSURE_S"` "
            "or a definition of type `"TI_VAL_STR_S"` "
            "but got type `%s` instead"DOC_T_TYPE,
            ti_val_str(val));

    return e->nr;
}

typedef struct
{
    ti_type_t * type;
    ex_t * e;
} type__init_t;

static int type__init_cb(ti_item_t * item, type__init_t * w)
{
    if (!ti_raw_is_name(item->key))
        ex_set(w->e, EX_VALUE_ERROR,
                "type keys must follow the naming rules"DOC_NAMES);
    else
        (void) type__assign(
                w->type,
                (ti_name_t *) item->key,
                item->val,
                w->e);
    return w->e->nr;
}

static int type__init_thing_o(ti_type_t * type, ti_thing_t * thing, ex_t * e)
{
    if (ti_thing_is_dict(thing))
    {
        type__init_t w = {
                .type = type,
                .e = e,
        };
        return smap_values(thing->items.smap, (smap_val_cb) type__init_cb, &w);
    }

    for (vec_each(thing->items.vec, ti_prop_t, prop))
        if (type__assign(type, prop->name, prop->val, e))
            return e->nr;
    return 0;
}

static int type__init_thing_t(ti_type_t * type, ti_thing_t * thing, ex_t * e)
{
    ti_name_t * name;
    ti_val_t * val;
    for (thing_t_each(thing, name, val))
        if (type__assign(type, name, val, e))
            return e->nr;
    return 0;
}

int ti_type_init_from_thing(ti_type_t * type, ti_thing_t * thing, ex_t * e)
{
    assert (thing->collection);  /* type are only created within a collection
                                    scope and all things inside a collection
                                    scope have the collection set; */
    assert (type->fields->n == 0);  /* no fields should exist */

    int enr = ti_thing_is_object(thing)
            ? type__init_thing_o(type, thing, e)
            : type__init_thing_t(type, thing, e);

    if (enr)
    {
        /* cleanup fields on error */
        ti_field_t * field;
        while ((field = vec_last(type->fields)))
            ti_field_remove(field);
    }

    return enr;
}

/*
 * TODO: (COMPAT) This function can be removed when we want to stop support
 * for changes containing type changes using the `old map` format.
 * (Changed in version 0.3.3, 19 December 2019)
 *
 * This function is called only in case when unpacking according the new
 * version has failed, so has zero impact on performance.
 */
static int type__deprecated_init_map(
        ti_type_t * type,
        mp_unp_t * up,
        mp_obj_t * obj,
        ex_t * e)
{
    ti_name_t * name;
    ti_raw_t * spec_raw;
    mp_obj_t mp_field, mp_spec;
    size_t i;

    log_warning(
        "reading fields for type `%s` using the deprecated `map` format",
        type->name);

    for (i = obj->via.sz; i--;)
    {
        if (mp_next(up, &mp_field) != MP_STR)
        {
            ex_set(e, EX_BAD_DATA,
                    "failed unpacking fields for type `%s`;"
                    "expecting each field name to be a `str` value",
                    type->name);
            return e->nr;
        }

        if (!ti_name_is_valid_strn(mp_field.via.str.data, mp_field.via.str.n))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "failed unpacking fields for type `%s`;"
                    "fields must follow the naming rules"DOC_NAMES,
                    type->name);
            return e->nr;
        }

        if (mp_next(up, &mp_spec) != MP_STR)
        {
            ex_set(e, EX_TYPE_ERROR,
                    "failed unpacking fields for type `%s`;"
                    "expecting each field definitions to be a `str` value",
                    type->name);
            return e->nr;
        }

        name = ti_names_get(mp_field.via.str.data, mp_field.via.str.n);
        spec_raw = ti_str_create(mp_spec.via.str.data, mp_spec.via.str.n);

        if (!name || !spec_raw ||
            !ti_field_create(name, spec_raw, type, e))
            goto failed;

        ti_decref(name);
        ti_decref(spec_raw);
    }

    return e->nr;

failed:
    if (!e->nr)
        ex_set_mem(e);

    ti_name_drop(name);
    ti_val_drop((ti_val_t *) spec_raw);

    return e->nr;
}

/*
 * TODO: (COMPAT) This function can be removed when we want to stop support
 * for changes containing type changes using the `old map` format.
 * (Changed in version 0.9.24, 25 November 2020)
 *
 * This function is called only in case when unpacking according the new
 * version has failed, so has zero impact on performance.
 */
static int type__deprecated_init_arr(
        ti_type_t * type,
        mp_unp_t * up,
        mp_obj_t * arr,
        ex_t * e,
        _Bool with_methods,
        _Bool with_wrap_only)
{
    ti_name_t * name;
    mp_obj_t obj, mp_name, mp_spec;
    size_t i;
    ti_val_t * val = NULL;
    ti_raw_t * spec_raw = NULL;
    ti_vup_t vup = {
            .isclient = false,
            .collection = type->types->collection,
            .up = up,
    };

    if (with_wrap_only)
    {
        /* Ugly fix for bug #120 */
        const char * keep = up->pt;
        mp_obj_t mp_wpo;

        for (i = arr->via.sz; i--;)
            mp_skip(up);

        mp_skip(up);  /* methods (key) */
        mp_skip(up);  /* methods (map) */

        if (mp_skip(up) != MP_STR || mp_next(up, &mp_wpo) != MP_BOOL)
        {
            ex_set(e, EX_BAD_DATA,
                        "failed unpacking methods for type `%s`;"
                        "expecting a boolean as wrap-only value",
                        type->name);
            return e->nr;
        }

        ti_type_set_wrap_only_mode(type, mp_wpo.via.bool_);
        up->pt = keep;
    }

    for (i = arr->via.sz; i--;)
    {
        if (mp_next(up, &obj) != MP_ARR || obj.via.sz != 2 ||
            mp_next(up, &mp_name) != MP_STR ||
            mp_next(up, &mp_spec) != MP_STR)
        {
            ex_set(e, EX_BAD_DATA,
                    "failed unpacking fields for type `%s`;"
                    "expecting an array with two string values",
                    type->name);
            return e->nr;
        }

        if (!ti_name_is_valid_strn(mp_name.via.str.data, mp_name.via.str.n))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "failed unpacking fields for type `%s`;"
                    "fields must follow the naming rules"DOC_NAMES,
                    type->name);
            return e->nr;
        }

        name = ti_names_get(mp_name.via.str.data, mp_name.via.str.n);
        spec_raw = ti_str_create(mp_spec.via.str.data, mp_spec.via.str.n);

        if (!name || !spec_raw ||
            !ti_field_create(name, spec_raw, type, e))
            goto failed;

        ti_decref(name);
        ti_decref(spec_raw);
    }

    if (!with_methods)
        return e->nr;  /* this implies that `with_wrap_only` is also false */

    if (mp_skip(up) != MP_STR || mp_next(up, &obj) != MP_MAP)
    {
        ex_set(e, EX_BAD_DATA,
                "failed unpacking methods for type `%s`;"
                "expecting the methods as a map",
                type->name);
        return e->nr;
    }

    for (i = obj.via.sz; i--;)
    {
        if (mp_next(up, &mp_name) != MP_STR)
        {
            ex_set(e, EX_BAD_DATA,
                    "failed unpacking methods for type `%s`;"
                    "expecting a map with a string a method name",
                    type->name);
            return e->nr;
        }

        if (!ti_name_is_valid_strn(mp_name.via.str.data, mp_name.via.str.n))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "failed unpacking methods for type `%s`;"
                    "methods must follow the naming rules"DOC_NAMES,
                    type->name);
            return e->nr;
        }

        name = ti_names_get(mp_name.via.str.data, mp_name.via.str.n);
        if (!name)
            goto failed;

        val = ti_val_from_vup_e(&vup, e);
        if (!val)
            goto failed;

        if (!ti_val_is_closure(val))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "failed unpacking methods for type `%s`;"
                    "methods must have type `"TI_VAL_CLOSURE_S"` as value "
                    "but got type `%s` instead",
                    type->name,
                    ti_val_str(val));
            goto failed;
        }

        if (ti_type_add_method(type, name, (ti_closure_t *) val, e))
            goto failed;

        ti_decref(name);
        ti_decref(val);
    }

    if (with_wrap_only)
    {
        mp_skip(up);  /* wrap-only key */
        mp_skip(up);  /* wrap-only value */
    }

    return 0;  /* success */

failed:
    if (!e->nr)
        ex_set_mem(e);

    ti_name_drop(name);
    ti_val_drop(val);
    ti_val_drop((ti_val_t *) spec_raw);

    return e->nr;
}


int ti_type_init_from_unp(
        ti_type_t * type,
        mp_unp_t * up,
        ex_t * e,
        _Bool with_methods,
        _Bool with_wrap_only)
{
    ti_name_t * name;
    mp_obj_t obj, mp_name, mp_spec;
    size_t i;
    ti_val_t * val = NULL;
    ti_raw_t * spec_raw = NULL;
    ti_vup_t vup = {
            .isclient = false,
            .collection = type->types->collection,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_BOOL)
    {
        if (obj.tp == MP_ARR)
            return type__deprecated_init_arr(
                    type,
                    up,
                    &obj,
                    e,
                    with_methods,
                    with_wrap_only);

        if (obj.tp == MP_MAP)
            return type__deprecated_init_map(type, up, &obj, e);

        ex_set(e, EX_BAD_DATA,
                "failed unpacking fields for type `%s`;"
                "expecting the field as an array",
                type->name);
        return e->nr;
    }

    /* It is important to set wrap-only mode before initializing,
     * since field dependencies might depend on the correct wrap-only
     * setting.
     */
    ti_type_set_wrap_only_mode(type, obj.via.bool_);

    if (mp_skip(up) != MP_STR || mp_next(up, &obj) != MP_ARR)
    {
        ex_set(e, EX_BAD_DATA,
                "failed unpacking fields for type `%s`;"
                "expecting the field as an array",
                type->name);
        return e->nr;
    }

    for (i = obj.via.sz; i--;)
    {
        if (mp_next(up, &obj) != MP_ARR || obj.via.sz != 2 ||
            mp_next(up, &mp_name) != MP_STR ||
            mp_next(up, &mp_spec) != MP_STR)
        {
            ex_set(e, EX_BAD_DATA,
                    "failed unpacking fields for type `%s`;"
                    "expecting an array with two string values",
                    type->name);
            return e->nr;
        }

        if (!ti_name_is_valid_strn(mp_name.via.str.data, mp_name.via.str.n))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "failed unpacking fields for type `%s`;"
                    "fields must follow the naming rules"DOC_NAMES,
                    type->name);
            return e->nr;
        }

        name = ti_names_get(mp_name.via.str.data, mp_name.via.str.n);
        spec_raw = ti_str_create(mp_spec.via.str.data, mp_spec.via.str.n);

        if (!name || !spec_raw ||
            !ti_field_create(name, spec_raw, type, e))
            goto failed;

        ti_decref(name);
        ti_decref(spec_raw);
    }

    if (mp_skip(up) != MP_STR || mp_next(up, &obj) != MP_MAP)
    {
        ex_set(e, EX_BAD_DATA,
                "failed unpacking methods for type `%s`;"
                "expecting the methods as a map",
                type->name);
        return e->nr;
    }

    for (i = obj.via.sz; i--;)
    {
        if (mp_next(up, &mp_name) != MP_STR)
        {
            ex_set(e, EX_BAD_DATA,
                    "failed unpacking methods for type `%s`;"
                    "expecting a map with a string a method name",
                    type->name);
            return e->nr;
        }

        if (!ti_name_is_valid_strn(mp_name.via.str.data, mp_name.via.str.n))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "failed unpacking methods for type `%s`;"
                    "methods must follow the naming rules"DOC_NAMES,
                    type->name);
            return e->nr;
        }

        name = ti_names_get(mp_name.via.str.data, mp_name.via.str.n);
        if (!name)
            goto failed;

        val = ti_val_from_vup_e(&vup, e);
        if (!val)
            goto failed;

        if (!ti_val_is_closure(val))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "failed unpacking methods for type `%s`;"
                    "methods must have type `"TI_VAL_CLOSURE_S"` as value "
                    "but got type `%s` instead",
                    type->name,
                    ti_val_str(val));
            goto failed;
        }

        if (ti_type_add_method(type, name, (ti_closure_t *) val, e))
            goto failed;

        ti_decref(name);
        ti_decref(val);
    }

    return 0;  /* success */

failed:
    if (!e->nr)
        ex_set_mem(e);

    ti_name_drop(name);
    ti_val_drop(val);
    ti_val_drop((ti_val_t *) spec_raw);

    return e->nr;
}

/* adds a map with key/value pairs */
int ti_type_fields_to_pk(ti_type_t * type, msgpack_packer * pk)
{
    if (msgpack_pack_array(pk, type->fields->n))
        return -1;

    for (vec_each(type->fields, ti_field_t, field))
    {
        if (msgpack_pack_array(pk, 2) ||
            mp_pack_strn(pk, field->name->str, field->name->n) ||
            mp_pack_strn(pk, field->spec_raw->data, field->spec_raw->n))
            return -1;
    }

    return 0;
}

/* adds a map with key/value pairs */
int ti_type_methods_to_pk(ti_type_t * type, msgpack_packer * pk)
{
    if (msgpack_pack_map(pk, type->methods->n))
        return -1;

    for (vec_each(type->methods, ti_method_t, method))
    {
        if (mp_pack_strn(pk, method->name->str, method->name->n) ||
            ti_closure_to_store_pk(method->closure, pk)
        ) return -1;
    }

    return 0;
}

/* adds a map with key/value pairs */
int ti_type_relations_to_pk(ti_type_t * type, msgpack_packer * pk)
{
    size_t n = 0;
    for (vec_each(type->fields, ti_field_t, field))
        if (ti_field_has_relation(field))
            ++n;

    if (msgpack_pack_map(pk, n))
        return -1;

    for (vec_each(type->fields, ti_field_t, field))
    {
        if (ti_field_has_relation(field))
        {
            ti_field_t * ofield = field->condition.rel->field;

            if (mp_pack_strn(pk, field->name->str, field->name->n) ||
                msgpack_pack_map(pk, 3) ||
                mp_pack_str(pk, "type") ||
                mp_pack_strn(
                        pk,
                        ofield->type->rname->data,
                        ofield->type->rname->n) ||

                mp_pack_str(pk, "property") ||
                mp_pack_strn(pk, ofield->name->str, ofield->name->n) ||

                mp_pack_str(pk, "definition") ||
                mp_pack_strn(pk, ofield->spec_raw->data, ofield->spec_raw->n))
                return -1;
        }
    }

    return 0;
}

/* adds a map with key/value pairs */
int ti_type_methods_info_to_pk(
        ti_type_t * type,
        msgpack_packer * pk,
        _Bool with_definition)
{
    if (msgpack_pack_map(pk, type->methods->n))
        return -1;

    for (vec_each(type->methods, ti_method_t, method))
    {
        ti_raw_t * doc = ti_method_doc(method);
        ti_raw_t * def;

        if (mp_pack_strn(pk, method->name->str, method->name->n) ||

            msgpack_pack_map(pk, 3 + !!with_definition) ||

            mp_pack_str(pk, "doc") ||
            mp_pack_strn(pk, doc->data, doc->n) ||

            (with_definition && (def = ti_method_def(method)) && (
                mp_pack_str(pk, "definition") ||
                mp_pack_strn(pk, def->data, def->n)
            )) ||

            mp_pack_str(pk, "with_side_effects") ||
            mp_pack_bool(pk, method->closure->flags & TI_CLOSURE_FLAG_WSE) ||

            mp_pack_str(pk, "arguments") ||
            msgpack_pack_array(pk, method->closure->vars->n))
            return -1;

        for (vec_each(method->closure->vars, ti_prop_t, prop))
            if (mp_pack_str(pk, prop->name->str))
                return -1;
    }
    return 0;
}

ti_val_t * ti_type_as_mpval(ti_type_t * type, _Bool with_definition)
{
    ti_raw_t * raw;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_raw_t), sizeof(ti_raw_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (ti_type_to_pk(type, &pk, with_definition))
    {
        msgpack_sbuffer_destroy(&buffer);
        return NULL;
    }

    raw = (ti_raw_t *) buffer.data;
    ti_raw_init(raw, TI_VAL_MPDATA, buffer.size);

    return (ti_val_t *) raw;
}

/*
 * Returns a vector with all the properties of a given `to` type when are
 * found and compatible with a `to` type.
 * The return is NULL when a memory allocation has occurred.
 */
vec_t * ti_type_map(ti_type_t * t_type, ti_type_t * f_type)
{
    ti_field_t * f_field;
    ti_mapping_t * mapping;
    vec_t * mappings = imap_get(t_type->t_mappings, f_type->type_id);
    if (mappings)
        return mappings;

    mappings = vec_new(t_type->fields->n);
    if (!mappings)
        return NULL;

    for (vec_each(t_type->fields, ti_field_t, t_field))
    {
        f_field = ti_field_by_name(f_type, t_field->name);
        if (!f_field)
            continue;

        if (ti_field_maps_to_field(t_field, f_field))
        {
            mapping = ti_mapping_new(t_field, f_field);
            if (!mapping)
            {
                vec_destroy(mappings, free);
                return NULL;
            }
            VEC_push(mappings, mapping);
        }
    }

    (void) imap_add(t_type->t_mappings, f_type->type_id, mappings);
    return mappings;
}

/*
 * Type must have been checked for `wrap_only` mode before calling this
 * function.
 */
ti_val_t * ti_type_dval(ti_type_t * type)
{
    ti_thing_t * thing = ti_thing_t_create(0, type, type->types->collection);
    if (!thing)
        return NULL;

    assert (!ti_type_is_wrap_only(type));

    for (vec_each(type->fields, ti_field_t, field))
    {
        ti_val_t * val = field->dval_cb(field);
        if (!val)
        {
            ti_thing_destroy(thing);
            return NULL;
        }

        ti_val_attach(val, thing, field);

        VEC_push(thing->items.vec, val);
    }

    return (ti_val_t *) thing;
}

typedef struct
{
    vec_t * vec;
    ex_t * e;
    ti_type_t * type;
    ti_thing_t * thing;
} type__convert_t;

/*
 * Note: prop may be of ti_item_t. Therefore be careful with using the name
 * as name->str might not be null terminated.
 */
int type__convert_cb(ti_prop_t * prop, type__convert_t * w)
{
    ti_field_t * field = ti_field_by_name(w->type, prop->name);
    ti_val_t ** vaddr;

    if (!field)
    {
        ex_set(w->e, EX_TYPE_ERROR,
                "conversion failed; "
                "type `%s` has no property `%.*s` "
                "but the thing you are trying to convert has",
                w->type->name, prop->name->n, prop->name->str);
        return -1;
    }

    if (ti_field_has_relation(field))
    {
        ex_set(w->e, EX_TYPE_ERROR,
                "conversion failed; property `%s` on type `%s` has a relation "
                "and can therefore not be used as a type to convert to",
                field->name->str, field->type->name);
        return -1;
    }

    if (ti_val_is_mut_locked(prop->val))
    {
        ex_set(w->e, EX_OPERATION,
                "conversion failed; property `%s` is in use",
                field->name->str);
        return -1;
    }

    /*
     * There should be no changes to this pointer. Only a `set` or `array` may
     * allocate new space and create a new pointer, but since they are checked
     * for a lock, only a single reference will exist and no copy will be made.
     */
    vaddr = (ti_val_t **) vec_get_addr(w->vec, field->idx);
    *vaddr = prop->val;
    return ti_field_make_assignable(field, vaddr, w->thing, w->e);
}

int ti_type_convert(
        ti_type_t * type,
        ti_thing_t * thing,
        ti_change_t * change,   /* might be NULL */
        ex_t * e)
{
    type__convert_t w = {
            .vec = vec_new(type->fields->n),
            .e = e,
            .type = type,
            .thing = thing,
    };

    if (!w.vec)
    {
        ex_set_mem(e);
        return e->nr;
    }

    /*
     * Make empty vector so we can use VEC_set(..) to fill the vector with
     * values.
     */
    vec_fill_null(w.vec);

    if (ti_thing_is_dict(thing))
    {
        if (smap_values(
                thing->items.smap,
                (smap_val_cb) type__convert_cb,
                &w))
            goto fail0;
    }
    else for (vec_each(thing->items.vec, ti_prop_t, prop))
        if (type__convert_cb(prop, &w))
            goto fail0;

    for (vec_each(type->fields, ti_field_t, field))
    {
        ti_val_t ** val = (ti_val_t **) vec_get_addr(w.vec, field->idx);
        if (!*val)
        {
            if (change)
            {
                ti_task_t * task = ti_task_get_task(change, thing);

                *val = field->dval_cb(field);
                if (!*val || !task || ti_task_add_set(
                        task,
                        (ti_raw_t *) field->name,
                        *val))
                {
                    ex_set_mem(e);
                    goto fail0;
                }
                ti_val_attach(*val, thing, field);
            }
            else
            {
                ex_set(e, EX_TYPE_ERROR,
                        "conversion failed; property `%s` is missing",
                        field->name->str);
                goto fail0;
            }
        }
    }

    ti_thing_o_items_destroy(thing);

    /* make sure the `dictionary` flag is removed */
    thing->flags &= ~TI_THING_FLAG_DICT;
    thing->type_id = type->type_id;
    thing->via.type = type;
    thing->items.vec = w.vec;
    return e->nr;

fail0:
    free(w.vec);
    return e->nr;
}

/*
 * Type must have been checked for `wrap_only` mode before calling this
 * function.
 */
ti_thing_t * ti_type_from_thing(ti_type_t * type, ti_thing_t * from, ex_t * e)
{
    _Bool is_last_ref = from->ref == 1;

    ti_thing_t * thing = ti_thing_t_create(0, type, type->types->collection);
    if (!thing)
    {
        ex_set_mem(e);
        return NULL;
    }

    assert (!ti_type_is_wrap_only(type));

    if (ti_thing_is_object(from))
    {
        ti_prop_t * prop;
        ti_val_t * val;
        for (vec_each(type->fields, ti_field_t, field))
        {

            prop = ti_thing_o_prop_weak_get(from, field->name);
            if (!prop)
            {
                val = field->dval_cb(field);
                if (!val)
                {
                    ex_set_mem(e);
                    goto failed;
                }

                ti_val_attach(val, thing, field);
            }
            else
            {
                val = prop->val;
                val->ref += !is_last_ref;

                if (ti_field_make_assignable(field, &val, thing, e))
                {
                    if (!is_last_ref)
                        ti_val_unsafe_gc_drop(val);
                    goto failed;
                }

                /*
                 * This is a hack, but safe, because `from` will definitely be
                 * destroyed after this call. Must replace the value because if
                 * the value is a set or array, the parent must be left alone.
                 */
                if (is_last_ref)
                    prop->val = (ti_val_t *) ti_nil_get();
            }
            VEC_push(thing->items.vec, val);
        }
    }
    else
    {
        if (from->via.type != type)
        {
            ex_set(e, EX_TYPE_ERROR,
                    "cannot create an instance of type `%s` from type `%s`"
                    DOC_NEW,
                    type->name,
                    from->via.type->name);
            goto failed;
        }

        for (vec_each(type->fields, ti_field_t, field))
        {
            ti_val_t * val = VEC_get(from->items.vec, field->idx);

            val->ref += !is_last_ref;

            if (ti_val_make_assignable(&val, thing, field, e))
            {
                if (!is_last_ref)
                    ti_val_unsafe_gc_drop(val);
                goto failed;
            }

            /*
             * This is a hack, but safe, because `from` will definitely be
             * destroyed after this call. Must replace the value because if
             * the value is a set or array, the parent must be left alone.
             */
            if (is_last_ref)
                VEC_set(from->items.vec, ti_nil_get(), field->idx);

            VEC_push(thing->items.vec, val);
        }
    }

    return thing;

failed:
    assert (e->nr);
    ti_thing_cancel(thing);
    return NULL;
}

static inline int type__test_wpo_cb(ti_type_t * haystack, ti_type_t * needle)
{
    if (ti_type_is_wrap_only(haystack))
        return 0;

    for (vec_each(haystack->fields, ti_field_t, field))
        if (field->spec == needle->type_id)
            return -1;

    return 0;
}

int ti_type_required_by_non_wpo(ti_type_t * type, ex_t * e)
{
    if (type->refcount &&
        imap_walk(type->types->imap, (imap_cb) type__test_wpo_cb, type))
        ex_set(e, EX_OPERATION,
            "type `%s` is required by at least one other type "
            "without having `wrap-only` mode enabled",
            type->name);

    return e->nr;
}

int ti_type_uses_wpo(ti_type_t * type, ex_t * e)
{
    for (vec_each(type->fields, ti_field_t, field))
    {
        if (field->spec < TI_SPEC_ANY)
        {
            ti_type_t * dep = ti_types_by_id(type->types, field->spec);

            if (ti_type_is_wrap_only(dep))
            {
                ex_set(e, EX_OPERATION,
                    "type `%s` is dependent on at least one type "
                    "with `wrap-only` mode enabled",
                    type->name);
                return e->nr;
            }
        }
    }
    return 0;
}
