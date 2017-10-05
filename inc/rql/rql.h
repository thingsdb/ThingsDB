/*
 * rql.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_H_
#define RQL_H_

typedef struct rql_s rql_t;

#include <uv.h>
#include <util/vec.h>
#include <rql/pool.h>
#include <rql/args.h>

struct rql_s
{
    uv_loop_t * loop;
    rql_pool_t * pool;
    rql_args_t * args;
};

rql_t * rql_create(void);
void rql_destroy(rql_t * rql);
void rql_setup_logger(rql_t * rql);

#endif /* RQL_H_ */
