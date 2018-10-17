/*
 * prop.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <string.h>
#include <stdlib.h>
#include <thingsdb.h>
#include <ti/prop.h>
#include <util/logger.h>

ti_prop_t * ti_prop_create(const char * name)
{
    size_t n = strlen(name) + 1;
    ti_prop_t * prop = malloc(sizeof(ti_prop_t) + n);
    if (!prop)
        return NULL;
    memcpy(prop->name, name, n);
    prop->ref = 1;
    return prop;
}

void ti_prop_drop(ti_prop_t * prop)
{
    if (prop && !--prop->ref)
    {
        smap_pop(thingsdb_get()->props, prop->name);
        ti_prop_destroy(prop);
    }
}

ti_prop_t * ti_prop_grab(ti_prop_t * prop)
{
    prop->ref++;
    return prop;
}
