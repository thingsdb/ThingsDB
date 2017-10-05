/*
 * elem.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <rql/elem.h>

rql_elem_t * rql_elem_create(uint64_t id)
{
    rql_elem_t * elem = (rql_elem_t *) malloc(sizeof(rql_elem_t));
    if (!elem) return NULL;

    elem->ref = 1;
    elem->id = id;

    return elem;
}


rql_elem_t * rql_elem_grab(rql_elem_t * elem)
{
    elem->ref++;
    return elem;
}

void rql_elem_drop(rql_elem_t * elem)
{
    if (!--elem->ref)
    {
        free(elem);
    }
}
