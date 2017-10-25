/*
 * inliners.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_INLINERS_H_
#define RQL_INLINERS_H_

#include <stdint.h>
#include <rql/node.h>
#include <rql/lookup.h>

static inline _Bool rql_has_id(rql_t * rql, uint64_t id);
static inline _Bool rql_node_has_id(
        rql_node_t * node,
        rql_lookup_t * lookup,
        uint64_t id);

static inline _Bool rql_has_id(rql_t * rql, uint64_t id)
{
    return rql_node_has_id(rql->node, rql->lookup, id);
}

static inline _Bool rql_node_has_id(
        rql_node_t * node,
        rql_lookup_t * lookup,
        uint64_t id)
{
    return lookup->mask_[id % lookup->n_] & (1 << node->id);
}

#endif /* RQL_INLINERS_H_ */
