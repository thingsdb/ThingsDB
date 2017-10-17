/*
 * lookup.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <rql/lookup.h>

rql_lookup_t * rql_lookup_create(rql_t * rql, uint8_t n, uint8_t redundancy)
{
    rql_lookup_t * lookup = (rql_lookup_t *) malloc(
            sizeof(rql_lookup_t) + n * sizeof(uint64_t));
    if (!lookup) return NULL;
    lookup->n = n;
    lookup->redundancy = redundancy;
    lookup->sz = (n < redundancy) ? n : redundancy;
    lookup->nodes = vec_new(lookup->sz * n);
    if (!lookup->nodes)
    {
        rql_lookup_destroy(lookup);
        return NULL;
    }
    return lookup;
}



/*
 *
 * def create_lookup(nodes, redundancy=3):
    n = min(nodes, redundancy)
    lookup = [[None] * n for _ in range(nodes)]
    for c in range(0, nodes):
        offset = -c
        for i, lup in enumerate(lookup):
            if ((i-offset) % nodes) < redundancy:
                found = True
            if found:
                for p in range(redundancy):
                    if lup[p] is None:
                        lup[p] = c
                        found = False
                        break
                else:
                    offset += 1

    return lookup
 */
