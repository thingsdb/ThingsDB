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
#include <inttypes.h>
#include <util/vec.h>
#include <rql/args.h>
#include <rql/cfg.h>

const uint8_t rql_def_redundancy = 3;
const char * rql_fn = "rql.qp";

rql_t * rql_create(void);
void rql_destroy(rql_t * rql);
void rql_init_logger(rql_t * rql);
int rql_init_fn(rql_t * rql);

struct rql_s
{
    char * fn;
    rql_args_t * args;
    rql_cfg_t * cfg;
    vec_t * dbs;
    vec_t * nodes;
    vec_t * users;
    uv_loop_t * loop;
    rql_node_t * node;
    uint8_t redundancy;  /* value 1..64 */
};

#endif /* RQL_H_ */
