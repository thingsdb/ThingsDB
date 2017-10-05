/*
 * cfg.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_CFG_H_
#define RQL_CFG_H_

typedef struct rql_node_s  rql_node_t;

#include <stddef.h>

struct rql_node_s
{
    char * rql_path;
};

rql_node_t * rql_node_create(void);
void rql_node_destroy(rql_raw_t * raw);

#endif /* RQL_CFG_H_ */
