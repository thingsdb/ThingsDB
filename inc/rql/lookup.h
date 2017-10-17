/*
 * lookup.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_LOOKUP_H_
#define RQL_LOOKUP_H_

typedef struct rql_lookup_s rql_lookup_t;

struct rql_lookup_s
{
    uint8_t redundancy;
    uint8_t n;          /* equal to nodes->n */
    uint8_t sz;         /* equal to min(n, redundancy) */
    vec_t * nodes;      /* legnth is equal to sz * n */
    uint64_t masks[];
};

#endif /* RQL_LOOKUP_H_ */
