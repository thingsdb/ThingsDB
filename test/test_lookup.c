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
        for (uint8_t n = 1; n < 10; n++)
        {
            for (uint8_t r = 1; r < 6; r++)
            {
                size_t m = (n < r) ? n : r;
                size_t sz = n * m;
                rql_lookup_t * lookup = rql_lookup_create(n, r, nodes);
                assert (lookup->nodes_->sz == sz);
                for (size_t i = 0; i < sz; i++)
                {
                    assert (vec_get(lookup->nodes_, i) != NULL);
                }
                rql_lookup_destroy(lookup);
            }
        }
    }

    /* test assignments */
    {
        uint8_t n = 5;
        uint8_t r = 3;
        size_t nids = 1000;
        size_t expected = (size_t) (((float) r / n) * nids);
        rql_lookup_t * lookup = rql_lookup_create(n, r, nodes);
        for (uint8_t i = 0; i < n; i++)
        {
            size_t a = 0;
            for (size_t id = 0; id < nids; id++)
            {
                a += rql_lookup_node_has_id(lookup, &entries[i], id);
            }
            assert (a == expected);
        }

        rql_lookup_destroy(lookup);
    }

    free(nodes);

    test_end(0);
    return 0;
}
