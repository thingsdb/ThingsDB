/*
 * elem.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <rql/elem.h>

rql_elem_t * rql_elem_create(uint64_t id, rql_kind_t * kind)
{
    rql_elem_t * elem = (rql_elem_t *) malloc(sizeof(rql_elem_t));

    if (!elem) return NULL;
    elem->id = id;
    elem->kind = rql_kind_grab(kind);
    elem->elems_n = 0;
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
        rql_kind_drop(elem->kind);

        for (uint32_t i = 0; i < elem->elems_n; i++)
        {
            rql_elem_drop(elem->elems[i]);
        }

    }
}
