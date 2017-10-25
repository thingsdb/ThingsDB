/*
 * item.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_ITEM_H_
#define RQL_ITEM_H_

typedef struct rql_item_s  rql_item_t;

#include <stdint.h>
#include <rql/prop.h>
#include <rql/val.h>

rql_item_t * rql_item_create(rql_prop_t * prop, rql_val_e tp, void * v);
rql_item_t * rql_item_weak_create(rql_prop_t * prop, rql_val_e tp, void * v);
void rql_item_destroy(rql_item_t * item);

struct rql_item_s
{
    rql_prop_t * prop;
    rql_val_t val;
};

#endif /* RQL_ITEM_H_ */
