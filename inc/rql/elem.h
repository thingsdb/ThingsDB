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
#include <rql/prop.h>
#include <rql/val.h>
#include <util/vec.h>

rql_elem_t * rql_elem_create(uint64_t id);
rql_elem_t * rql_elem_grab(rql_elem_t * elem);
void rql_elem_drop(rql_elem_t * elem);
int rql_elem_set(rql_elem_t * elem, rql_prop_t * prop, rql_val_e tp, void * v);
int rql_elem_weak_set(
        rql_elem_t * elem,
        rql_prop_t * prop,
        rql_val_e tp,
        void * v);

struct rql_elem_s
{
    uint32_t ref;
    uint8_t flags;
    uint8_t pad0;
    uint16_t pad1;
    uint64_t id;
    vec_t * items;
};

#endif /* RQL_ELEM_H_ */
