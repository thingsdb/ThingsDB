/*
 * test_lookup.c
 *
 *  Created on: Oct 18, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "test.h"
#include "test_lookup.h"
#include <rql/lookup.h>

const unsigned int num_entries = 10;
rql_node_t entries[] = {
    {.id=0},
    {.id=1},
    {.id=2},
    {.id=3},
    {.id=4},
    {.id=5},
    {.id=6},
    {.id=7},
    {.id=8},
    {.id=9},
};

int main()
{
    test_start("lookup");

    /* setup nodes */
    vec_t * nodes = vec_new(num_entries);
    for (uint32_t i = 0; i < num_entries; i++)
    {
        assert (vec_push(&nodes, &entries[i]) == 0);
    }

    /* test create lookup */
    {
        rql_lookup_t * lookup = rql_lookup_create(5, 3, nodes);
        // for (vec_each(lookup->nodes_, rql_node_t, nd))
        // {
        //     printf("Node: %u\n", nd->id);
        // }
        // for (int i = 0; i < 5; i++)
        // {
        //     printf("Mask: %lu\n", lookup->mask_[i]);
        // }
        rql_lookup_destroy(lookup);
    }

    free(nodes);

    test_end(0);
    return 0;
}
