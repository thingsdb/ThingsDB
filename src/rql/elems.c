/*
 * elems.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <rql/elems.h>

rql_elem_t * rql_elems_create(imap_t * elems, uint64_t id)
{
    rql_elem_t * elem = rql_elem_create(id);
    if (!elem || imap_add(elems, id, elem))
    {
        rql_elem_drop(elem);
        return NULL;
    }
    return elem;
}
