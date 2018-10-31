#include "../test.h"
#include <ti/lookup.h>

static const unsigned int num_entries = 10;
static ti_node_t entries[] = {
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
        _assert (vec_push(&nodes, &entries[i]) == 0);
    }

    /* test create lookup */
    {
        for (uint8_t n = 1; n < 10; n++)
        {
            for (uint8_t r = 1; r < 6; r++)
            {
                size_t m = (n < r) ? n : r;
                size_t sz = n * m;
                ti_lookup_t * lookup = ti_lookup_create(n, r, nodes);
                _assert (lookup->nodes_->sz == sz);
                for (size_t i = 0; i < sz; i++)
                {
                    _assert (vec_get(lookup->nodes_, i) != NULL);
                }
                ti_lookup_destroy(lookup);
            }
        }
    }

    /* test assignments */
    {
        uint8_t n = 5;
        uint8_t r = 3;
        size_t nids = 1000;
        size_t expected = (size_t) (((float) r / n) * nids);
        ti_lookup_t * lookup = ti_lookup_create(n, r, nodes);
        for (uint8_t i = 0; i < n; i++)
        {
            size_t a = 0;
            for (size_t id = 0; id < nids; id++)
            {
                a += ti_lookup_node_has_id(lookup, &entries[i], id);
            }
            _assert (a == expected);
        }

        ti_lookup_destroy(lookup);
    }

    free(nodes);

    return test_end();
}
