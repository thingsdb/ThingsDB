/*
 * ti/type.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <doc.h>
#include <stdbool.h>
#include <ti/type.h>
#include <ti/names.h>
#include <ti/prop.h>
#include <ti/thingi.h>
#include <ti/field.h>

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
    type->name = strndup(name, n);
    type->name_n = n;
    type->dependencies = vec_new(0);
    type->fields = vec_new(0);
    type->types = types;
    type->t_mappings = imap_create();

    if (!type->name || !type->dependencies || !type->fields ||
        !type->t_mappings || ti_types_add(types, type))
    {
        ti_type_destroy(type);
        return NULL;
    }

    return type;
}

static int type__map_cleanup(ti_type_t * t_haystack, ti_type_t * t_needle)
{
    (void) imap_pop(t_haystack->t_mappings, t_needle->type_id);
    return 0;
}

void ti_type_map_cleanup(ti_type_t * type)
{
    (void) imap_walk(type->types->imap, (imap_cb) type__map_cleanup, type);
}

void ti_type_drop(ti_type_t * type)
{
    if (!type)
        return;

    for (vec_each(type->dependencies, ti_type_t, dep))
        --dep->refcount;

    ti_type_map_cleanup(type);
    ti_types_del(type->types, type);
    ti_type_destroy(type);
}

void ti_type_destroy(ti_type_t * type)
{
    if (!type)
        return;

    vec_destroy(type->fields, (vec_destroy_cb) ti_field_destroy);
    imap_destroy(type->t_mappings, free);
    free(type->dependencies);
    free(type->name);
    free(type);
}

size_t ti_type_approx_pack_sz(ti_type_t * type)
{
    size_t n = type->name_n + 32;  /* 'name', 'type_id', 'fields', type_id */
    for (vec_each(type->fields, ti_field_t, field))
    {
        n += field->name->n + field->spec_raw->n + 10;
    }
    return n;
}

_Bool ti_type_is_valid_strn(const char * str, size_t n)
{
    if (!n || n > TI_TYPE_NAME_MAX || (!isupper(*str)))
        return false;

    while(--n)
        if (!isalnum(str[n]) && str[n] != '_')
            return false;

    return true;
}

static inline int type__field(
        ti_type_t * type,
        ti_name_t * name,
        ti_val_t * val,
        ex_t * e)
{
    if (!ti_val_is_raw(val))
    {
        ex_set(e, EX_TYPE_ERROR,
                "expecting a type definition to be type `"TI_VAL_RAW_S"` "
                "but got type `%s` instead"DOC_SPEC,
                ti_val_str(val));
        return e->nr;
    }
    return ti_field_create(name, (ti_raw_t *) val, type, e);
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

int ti_type_init_from_unp(ti_type_t * type, qp_unpacker_t * unp, ex_t * e)
{
    ti_name_t * name;
    ti_raw_t * spec_raw;
    qp_obj_t qp_field, qp_spec;
    int mapsz;
    const char * field_name;
    size_t field_n;

    if (!qp_is_map(mapsz = qp_next(unp, NULL)))
    {
        ex_set(e, EX_BAD_DATA,
                "failed unpacking fields for type `%s`;"
                "expecting `type` to start with a map",
                type->name);
        return e->nr;
    }

    mapsz = mapsz == QP_MAP_OPEN ? -1 : mapsz - QP_MAP0;

    while (mapsz--)
    {
        if (qp_is_close(qp_next(unp, &qp_field)))
            break;

        if (!qp_is_raw(qp_field.tp))
            ex_set(e, EX_BAD_DATA,
                    "failed unpacking fields for type `%s`;"
                    "expecting each field name to be a `raw` value",
                    type->name);

        field_name = (const char *) qp_field.via.raw;
        field_n = qp_field.len;

        if (!ti_name_is_valid_strn(field_name, field_n))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "failed unpacking fields for type `%s`;"
                    "fields must follow the naming rules"DOC_NAMES,
                    type->name);
            return e->nr;
        }

        if (!qp_is_raw(qp_next(unp, &qp_spec)))
        {
            ex_set(e, EX_TYPE_ERROR,
                    "failed unpacking fields for type `%s`;"
                    "expecting each field definitions to be a `raw` value",
                    type->name);
            return e->nr;
        }

        name = ti_names_get(field_name, field_n);
        spec_raw = ti_raw_create(qp_spec.via.raw, qp_spec.len);

        if (!name || !spec_raw ||
            ti_field_create(name, spec_raw, type, e))
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

/* adds a key/value pair, which is supposed to be added to a `map`. */
int ti_type_fields_to_packer(ti_type_t * type, qp_packer_t ** packer)
{
    if (qp_add_map(packer))
        return -1;

    for (vec_each(type->fields, ti_field_t, field))
    {
        if (qp_add_raw(
                *packer,
                (const uchar *) field->name->str,
                field->name->n) ||
            qp_add_raw(*packer, field->spec_raw->data, field->spec_raw->n))
            return -1;
    }

    return qp_close_map(*packer);
}

ti_val_t * ti_type_info_as_qpval(ti_type_t * type)
{
    ti_raw_t * rtype = NULL;
    size_t alloc_sz = ti_type_approx_pack_sz(type) - type->name_n - 25;
    qp_packer_t * packer = qp_packer_create2(alloc_sz, 2);
    if (!packer)
        return NULL;

    (void) ti_type_fields_to_packer(type, &packer);

    rtype = ti_raw_from_packer(packer);

    qp_packer_destroy(packer);
    return (ti_val_t * ) rtype;
}

static ti_field_t * type__field_by_name(
        ti_type_t * to_type,
        ti_name_t * name,
        uintptr_t * idx)
{
    *idx = 0;
    for (vec_each(to_type->fields, ti_field_t, field), ++(*idx))
        if (field->name == name)
            return field;
    return NULL;
}

/*
 * Returns a vector with a size equal to `to_type->fields` and each item in
 * the vector contains an index in the `from_type->field` where to find a
 * corresponding value;
 *
 * If, and only if the return value is NULL, then `e` is set to an error
 * message;
 */
vec_t * ti_type_map(ti_type_t * t_type, ti_type_t * f_type, ex_t * e)
{
    uintptr_t idx;
    ti_field_t * f_field;
    vec_t * t_map = imap_get(t_type->t_mappings, f_type->type_id);
    if (t_map)
        return t_map;

    t_map = vec_new(t_type->fields->n);
    if (!t_map)
        goto failed;

    for (vec_each(t_type->fields, ti_field_t, t_field))
    {
        f_field = type__field_by_name(f_type, t_field->name, &idx);
        if (!f_field)
        {
            ex_set(e, EX_LOOKUP_ERROR,
                    "invalid cast from `%s` to `%s`; "
                    "missing property `%s` in type `%s`",
                    f_type->name,
                    t_type->name,
                    t_field->name->str,
                    f_type->name);
            goto failed;
        }

        if (ti_field_check_field(t_field, f_field, e))
            goto failed;

        VEC_push(t_map, (void *) idx);
    }

    if (imap_add(t_type->t_mappings, f_type->type_id, t_map) == 0)
        return t_map;

failed:
    if (!e->nr)
        ex_set_mem(e);
    free(t_map);
    return NULL;
}
