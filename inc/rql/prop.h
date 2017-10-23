/*
 * prop.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_PROP_H_
#define RQL_PROP_H_

typedef struct rql_prop_s rql_prop_t;

#include <inttypes.h>

rql_prop_t * rql_prop_create(const char * name);
void rql_prop_grab(rql_prop_t * prop);

struct rql_prop_s
{
    uint64_t ref;
    char name[];
};

#endif /* RQL_PROP_H_ */
