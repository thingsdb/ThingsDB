/*
 * name.c
 */
#include <string.h>
#include <stdlib.h>
#include <ti/name.h>
#include <ti.h>
#include <util/logger.h>

ti_name_t * ti_name_create(const char * str, size_t n)
{
    size_t sz = n + 1;
    ti_name_t * name = malloc(sizeof(ti_name_t) + sz);
    if (!name)
        return NULL;

    memcpy(name->str, str, n);
    name->str[sz] = '\0';
    name->n = n;
    name->ref = 1;
    return name;
}

void ti_name_drop(ti_name_t * name)
{
    if (name && !--name->ref)
    {
        smap_pop(ti_get()->names, name->str);
        ti_name_destroy(name);
    }
}
