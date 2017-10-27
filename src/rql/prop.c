/*
 * prop.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <string.h>
#include <stdlib.h>
#include <rql/prop.h>
#include <util/logger.h>

rql_prop_t * rql_prop_create(const char * name)
{
    size_t n = strlen(name) + 1;
    rql_prop_t * prop = (rql_prop_t *) malloc(sizeof(rql_prop_t) + n);
    memcpy(prop->name, name, n);
    prop->ref = 1;
    return prop;
}

rql_prop_t * rql_prop_grab(rql_prop_t * prop)
{
    prop->ref++;
    return prop;
}

void rql_prop_drop(rql_prop_t * prop)
{
    if (prop && !--prop->ref)
    {
        LOGC("DROP prop...");
        free(prop);
    }
}
