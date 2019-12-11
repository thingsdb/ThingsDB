
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
#include <ti/field.h>
#include <ti/mapping.h>
#include <ti/names.h>
#include <ti/prop.h>
#include <ti/raw.inline.h>
#include <ti/thing.inline.h>
#include <ti/type.h>

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
        const char * name,
        size_t n)
{
    ti_type_t * type = malloc(sizeof(ti_type_t));
    if (!type)
        return NULL;

    type->refcount = 0;
    type->type_id = type_id;
    type->flags = 0;
    type->name = strndup(name, n);
    type->wname = type__wrap_name(name, n);
    type->rname = ti_str_create(name, n);
    type->rwname = ti_str_from_str(type->wname);
    type->dependencies = vec_new(0);
    type->fields = vec_new(0);
    type->types = types;
    type->t_mappings = imap_create();

    if (!type->name || !type->wname || !type->dependencies || !type->fields ||
        !type->t_mappings || ti_types_add(types, type))
    {
        ti_type_destroy(type);
        return NULL;
    }

    return type;
}

static int type__map_cleanup(ti_type_t * t_haystack, ti_type_t * t_needle)
{
    vec_destroy(imap_pop(t_haystack->t_mappings, t_needle->type_id), free);
    return 0;
}

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

static int type__conv(ti_thing_t * thing, uint16_t * type_id)
{
    if (thing->type_id == *type_id)
        ti_thing_t_to_object(thing);
    return 0;
}

void ti_type_del(ti_type_t * type)
{
    assert (!type->refcount);

    ti_collection_t * collection = type->types->collection;
    uint16_t type_id = type->type_id;

    (void) imap_walk(collection->things, (imap_cb) type__conv, &type_id);

    ti_type_drop(type);
}

void ti_type_destroy(ti_type_t * type)
{
    if (!type)
        return;

    vec_destroy(type->fields, (vec_destroy_cb) ti_field_destroy);
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

static inline int type__field(
        ti_type_t * type,
        ti_name_t * name,
        ti_val_t * val,
        ex_t * e)
{
    if (!ti_val_is_str(val))
    {
        ex_set(e, EX_TYPE_ERROR,
                "expecting a type definition to be type `"TI_VAL_STR_S"` "
                "but got type `%s` instead"DOC_SPEC,
                ti_val_str(val));
        return e->nr;
    }
    (void) ti_field_create(name, (ti_raw_t *) val, type, e);
    return e->nr;
}

static int type__init_thing_o(ti_type_t * type, ti_thing_t * thing, ex_t * e)
{
    for (vec_each(thing->items, ti_prop_t, prop))
        if (type__field(type, prop->name, prop->val, e))
            return e->nr;
    return 0;
}

static int type__init_thing_t(ti_type_t * type, ti_thing_t * thing, ex_t * e)
{
    ti_name_t * name;
    ti_val_t * val;
    for (thing_each(thing, name, val))
        if (type__field(type, name, val, e))
            return e->nr;
    return 0;
}

int ti_type_init_from_thing(ti_type_t * type, ti_thing_t * thing, ex_t * e)
{
    assert (thing->collection);  /* type are only created within a collection
                                    scope and all things inside a collection
                                    scope have the collection set; */
    return ti_thing_is_object(thing)
            ? type__init_thing_o(type, thing, e)
            : type__init_thing_t(type, thing, e);
}

int ti_type_init_from_unp(ti_type_t * type, mp_unp_t * up, ex_t * e)
{
    ti_name_t * name;
    ti_raw_t * spec_raw;
    mp_obj_t obj, mp_field, mp_spec;
    size_t i;

    if (mp_next(up, &obj) != MP_MAP)
    {
        ex_set(e, EX_BAD_DATA,
                "failed unpacking fields for type `%s`;"
                "expecting `type` to start with a map",
                type->name);
        return e->nr;
    }

    for (i = obj.via.sz; i--;)
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

/* adds a map with key/value pairs */
int ti_type_fields_to_pk(ti_type_t * type, msgpack_packer * pk)
{
    if (msgpack_pack_map(pk, type->fields->n))
        return -1;

    for (vec_each(type->fields, ti_field_t, field))
    {
        if (mp_pack_strn(pk, field->name->str, field->name->n) ||
            mp_pack_strn(pk, field->spec_raw->data, field->spec_raw->n))
            return -1;
    }

    return 0;
}

ti_val_t * ti_type_as_mpval(ti_type_t * type)
{
    ti_raw_t * raw;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_raw_t), sizeof(ti_raw_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (ti_type_fields_to_pk(type, &pk))
    {
        msgpack_sbuffer_destroy(&buffer);
        return NULL;
    }

    raw = (ti_raw_t *) buffer.data;
    ti_raw_init(raw, TI_VAL_MP, buffer.size);

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
