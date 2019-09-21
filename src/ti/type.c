/*
 * ti/type.h
 */
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <ti/type.h>

ti_type_t * ti_type_create(uint16_t class, const char * name, size_t n)
{
    ti_type_t * type = malloc(sizeof(ti_type_t));
    if (!type)
        return NULL;

    type->refcount = 0;
    type->class = class;
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

void ti_type_destroy(ti_type_t * type)
{
    if (!type)
        return;

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

ti_type_t * ti_type_from_thing(ti_thing_t * thing)
{

}
