/*
 * kind.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_KIND_H_
#define RQL_KIND_H_

typedef struct rql_kind_s rql_kind_t;

#include <inttypes.h>
#include <rql/prop.h>

rql_kind_t * rql_kind_create(const char * name);
rql_kind_t * rql_kind_grab(rql_kind_t * kind);
void rql_kind_drop(rql_kind_t * kind);
int rql_kind_append_props(rql_kind_t * kind, rql_prop_t * props[], uint32_t n);

struct rql_kind_s
{
    uint64_t ref;
    uint32_t flags;
    vec_t * props;
    char * name;
};

#endif /* RQL_KIND_H_ */
