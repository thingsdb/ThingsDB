/*
 * pool.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_POOL_H_
#define RQL_POOL_H_

typedef struct rql_pool_s  rql_pool_t;

#include <stddef.h>
#include <util/vec.h>

rql_pool_t * rql_pool_create(void);
void rql_pool_destroy(rql_pool_t * pool);

struct rql_pool_s
{
    vec_t * dbs;
    vec_t * nodes;
};

#endif /* RQL_POOL_H_ */
