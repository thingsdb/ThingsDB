/*
 * ti/type.h
 */
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ti/type.h>

ti_type_t * ti_type_create(
        ti_types_t * types,
        uint16_t id,
        const char * name,
        size_t n)
{
    ti_type_t * type = malloc(sizeof(ti_type_t));
    if (!type)
        return NULL;

    type->ref =
    type->name = strndup(name, n);
    type->name_n = n;
    type->dependencies = vec_new(0);
    type->fields = vec_new(0);

    imap_add(types->imap, id, type);  /* must be 0 */
    smap_add(types->smap, name);

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
    if (!n || (!isupper(str[0])))
        return false;

    while(--n)
        if (!isalnum(str[n]) && str[n] != '_')
            return false;

    return true;
}
