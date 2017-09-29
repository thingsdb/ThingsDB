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
typedef union rql_prop_u rql_prop_via_t;

#include <inttypes.h>
#include <rql/val.h>
#include <rql/kind.h>

rql_prop_t * rql_prop_create(
        const char * name,
        rql_val_t tp,
        uint32_t flags,
        rql_prop_via_t via);
void rql_prop_destroy(rql_prop_t * prop);

union rql_prop_u
{
    rql_kind_t * kind;   /* NULL or restricted to */
    rql_val_via_t * def;  /* defaults can only be used with strings etc. */
};

struct rql_prop_s
{
    char * name;
    rql_val_t tp;
    uint32_t flags;
    rql_prop_via_t via;
};

#endif /* RQL_PROP_H_ */
