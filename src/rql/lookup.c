/*
 * lookup.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */



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
