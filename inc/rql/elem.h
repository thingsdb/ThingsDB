/*
 * elem.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_ELEM_H_
#define RQL_ELEM_H_

typedef struct rql_elem_s  rql_elem_t;

#include <inttypes.h>

rql_elem_t * rql_elem_create(uint64_t id);
rql_elem_t * rql_elem_grab(rql_elem_t * elem);
void rql_elem_drop(rql_elem_t * elem);

struct rql_elem_s
{
    uint64_t ref;
    uint64_t id;
};

#endif /* RQL_ELEM_H_ */
