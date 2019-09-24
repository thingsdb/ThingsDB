/*
 * ti/type.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <doc.h>
#include <stdbool.h>
#include <ti/type.h>
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

}
