/*
 * prop.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_PROP_H_
#define RQL_PROP_H_

/* flags */
#define RQL_PROP_IS_ARR 1
#define RQL_PROP_NO_NULL 2

typedef struct rql_prop_s  rql_prop_t;

#include <inttypes.h>
#include <rql/val.h>
#include <rql/kind.h>

rql_prop_t * rql_prop_create(
        const char * name,
        rql_val_t tp,
        uint32_t flags,
        rql_kind_t * kind,
        rql_prop_t * fwd_prop,
        rql_val_via_t * def);
void rql_prop_destroy(rql_prop_t * prop);

struct rql_prop_s
{
    char * name;
    rql_val_t tp;
    uint32_t flags;
    rql_kind_t * kind;   /* NULL or restricted to */
    rql_prop_t * prop;   /* Can be used when kind is set */
    rql_val_via_t * def;  /* defaults can only be used with strings etc. */
};

#endif /* RQL_PROP_H_ */
