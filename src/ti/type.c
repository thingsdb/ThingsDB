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

ti_type_t * ti_type_create(uint16_t type_id, const char * name, size_t n)
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

    if (!type->name || !type->dependencies || !type->fields)
    {
        ti_type_destroy(type);
        return NULL;
    }

    return type;
}

static void type__decref(ti_type_t * type)
{
    --type->refcount;
}

void ti_type_destroy(ti_type_t * type)
{
    if (!type)
        return;

    vec_destroy(type->dependencies, (vec_destroy_cb) type__decref);
    vec_destroy(type->fields, (vec_destroy_cb) ti_field_destroy);

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
    if (!n || (!isupper(*str)))
        return false;

    while(--n)
        if (!isalnum(str[n]) && str[n] != '_')
            return false;

    return true;
}

static inline int type__field(
        ti_type_t * type,
        ti_types_t * types,
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
    return ti_field_create(name, (ti_raw_t *) val, type, types, e);
}


static int type__init_thing_o(ti_type_t * type, ti_thing_t * thing, ex_t * e)
{
    ti_types_t * types = thing->collection->types;
    for (vec_each(thing->items, ti_prop_t, prop))
        if (type__field(type, types, prop->name, prop->val, e))
            return e->nr;
    return 0;
}

static int type__init_thing_t(ti_type_t * type, ti_thing_t * thing, ex_t * e)
{
    ti_types_t * types = thing->collection->types;
    ti_name_t * name;
    ti_val_t * val;
    for (thing_each(thing, name, val))
        if (type__field(type, types, name, val, e))
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

int ti_type_init_from_unp(
        ti_type_t * type,
        ti_types_t * types,
        qp_unpacker_t * unp,
        ex_t * e)
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

    while (mapsz-- && qp_is_raw(qp_next(unp, &qp_field)))
    {
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
            ti_field_create(name, spec_raw, type, types, e))
            goto failed;

        ti_decref(name);
        ti_decref(spec_raw);
    }

    if (!qp_is_raw(qp_field.tp) && !qp_is_close(qp_field.tp))
        ex_set(e, EX_BAD_DATA,
                "failed unpacking fields for type `%s`;"
                "expecting each field name to be a `raw` value",
                type->name);

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

/*
 * Returns a vector with a size equal to `to_type->fields` and each item in
 * the vector contains an index in the `from_type->field` where to find a
 * corresponding value;
 *
 * If, and only if the return value is NULL, then `e` is set to an error
 * message;
 */
vec_t * ti_type_cast(ti_type_t * to_type, ti_type_t * from_type, ex_t * e)
{
    ti_field_t * from_field;
    vec_t * cast = imap_get(to_type->casts, from_type->type_id);
    if (cast)
        return cast;

    cast = vec_new(to_type->fields->n);
    if (!cast)
        goto failed;

    for (vec_each(to_type->fields, ti_field_t, to_field))
    {
        from_field = type__field_by_name(from_type, to_field->name);
        if (!from_field)
        {
            ex_set(e, EX_LOOKUP_ERROR,
                    "invalid cast from `%s` to `%s`;"
                    "missing property `%s` in type `%s`",
                    from_type->name,
                    to_type->name,
                    to_field->name->str,
                    from_type->name);
            goto failed;
        }
    }

    if (imap_add(to_type->casts, from_type->type_id, cast) == 0)
        return cast;

failed:
    if (!e->nr)
        ex_set_mem(e);
    free(cast);
    return NULL;
}
