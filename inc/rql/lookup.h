/*
 * lookup.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_LOOKUP_H_
#define RQL_LOOKUP_H_

typedef struct rql_lookup_s rql_lookup_t;

#include <inttypes.h>
#include <util/vec.h>

#ifndef TESTING
#include <rql/node.h>
#else
#include "../test/test_lookup.h"
#endif

rql_lookup_t * rql_lookup_create(
        uint8_t n,
        uint8_t redundancy,
        const vec_t * nodes);
void rql_lookup_destroy(rql_lookup_t * lookup);
static inline _Bool rql_lookup_node_has_id(
        rql_lookup_t * lookup,
        rql_node_t * node,
        uint64_t id);

struct rql_lookup_s
{
    uint8_t n_;         /* equal to nodes->n */
    uint8_t r_;         /* equal to min(n, redundancy) */
    vec_t * nodes_;     /* length is equal to r_ * n */
    uint64_t mask_[];
};

static inline _Bool rql_lookup_node_has_id(
        rql_lookup_t * lookup,
        rql_node_t * node,
        uint64_t id)
{
//    return lookup->mask_[id % lookup->n_] & (1 << node->id);
}

#endif /* RQL_LOOKUP_H_ */
