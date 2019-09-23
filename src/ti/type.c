/*
 * ti/type.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <ti/type.h>
#include <ti/prop.h>
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

_Bool ti_type_is_valid_strn(const char * str, size_t n)
{
    if (!n || (!isupper(*str)))
        return false;

    while(--n)
        if (!isalnum(str[n]) && str[n] != '_')
            return false;

    return true;
}


static int type__init_thing_o(ti_type_t * type, ti_thing_t * thing, ex_t * e)
{
    for (vec_each(thing->items, ti_prop_t, prop))
    {
        ti_field_t * field = ti_field_create(prop->name,)
    }
}

static int type__init_thing_t(ti_type_t * type, ti_thing_t * thing, ex_t * e)
{

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
