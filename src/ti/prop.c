/*
 * prop.c
 */
#include <string.h>
#include <stdlib.h>
#include <ti/prop.h>
#include <ti.h>
#include <util/logger.h>

ti_prop_t * ti_prop_create(const char * name, size_t n)
{
    size_t sz = n + 1;
    ti_prop_t * prop = malloc(sizeof(ti_prop_t) + sz);
    if (!prop)
        return NULL;

    memcpy(prop->name, name, n);
    prop->name[sz] = '\0';
    prop->n = n;
    prop->ref = 1;
    return prop;
}

ti_prop_t * ti_prop_grab(ti_prop_t * prop)
{
    prop->ref++;
    return prop;
}

void ti_prop_drop(ti_prop_t * prop)
{
    if (prop && !--prop->ref)
    {
        smap_pop(ti_get()->props, prop->name);
        ti_prop_destroy(prop);
    }
}
