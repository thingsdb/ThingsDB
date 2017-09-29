/*
 * arr.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_ARR_H_
#define RQL_ARR_H_

typedef struct rql_arr_s  rql_arr_t;

#include <inttypes.h>
#include <rql/val.h>

struct rql_arr_s
{
    uint32_t n;
    uint32_t sz;
    rql_val_via_t * values;
};

#endif /* RQL_VAL_H_ */
