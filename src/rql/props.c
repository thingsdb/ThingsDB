/*
 * props.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <rql/props.h>

rql_prop_t * rql_db_props_get(smap_t * props, const char * name)
{
    rql_prop_t * prop = (rql_prop_t *) smap_get(props, name);
    if (!prop)
    {
        prop = rql_prop_create(name);
        if (!prop || smap_add(props, name, prop))
        {
            rql_prop_drop(prop);
            return NULL;
        }
    }
    return prop;
}
