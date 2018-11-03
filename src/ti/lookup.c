/*
 * ti/lookup.c
 */
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <ti/lookup.h>
#include <ti.h>

static ti_lookup_t * lookup;

static void lookup__calculate(const vec_t * nodes);

int ti_lookup_create(uint8_t redundancy, const vec_t * vec_nodes)
{
    size_t n = vec_nodes->n;

    assert (n);

    lookup = malloc(sizeof(ti_lookup_t) + n * sizeof(uint64_t));
    if (!lookup)
        return -1;

    lookup->n_ = n;
    lookup->r_ = (n < redundancy) ? n : redundancy;
    lookup->nodes_ = vec_new(lookup->r_ * n);
    lookup->cache_ = malloc(sizeof(uint8_t) * n);
    lookup->tmp_ = malloc(sizeof(uint8_t) * (n - 1));

    lookup->factorial_ = 1;
    for(size_t i = 1; i <= n; ++i)
        lookup->factorial_ *= i;

    if (!lookup->nodes_ || !lookup->cache_ || !lookup->tmp_)
    {
        ti_lookup_destroy();
        return -1;
    }

    lookup__calculate(vec_nodes);

    ti()->lookup = lookup;

    return 0;
}

void ti_lookup_destroy(void)
{
    if (!lookup)
        return;
    free(lookup->nodes_);
    free(lookup->cache_);
    free(lookup->tmp_);
    free(lookup);
    ti()->lookup = lookup = NULL;
}

_Bool ti_lookup_node_has_id(ti_node_t * node, uint64_t id)
{
    return lookup->mask_[id % lookup->n_] & (1 << node->id);
}

/* TODO: need testing */
_Bool ti_lookup_node_is_ordered(uint8_t a, uint8_t b, uint64_t u)
{
    uint8_t i, n = lookup->n_;
    size_t m, f = lookup->factorial_;

    assert (a < b);
    assert (b < lookup->n_);

    for (i = 0; i < n; ++i)
        lookup->cache_[i] = i;

    for (m = u % f; i > 1; m %= f, --i)
    {
        f /= i;
        lookup->tmp_[n-i] = (uint8_t) (m / f);
    }

    for (i = 0; true; ++i)
    {
        uint8_t p = lookup->tmp_[i] + i;
        n = lookup->cache_[p];

        if (n == a)
            return true;

        if (n == b)
            return false;

        lookup->cache_[p] = lookup->cache_[i];
        lookup->cache_[i] = n;
    }

    assert (0);
    return 0;
}

static void lookup__calculate(const vec_t * nodes)
{
    /* set lookup to NULL */
    for (uint32_t i = 0, sz = lookup->nodes_->sz; i < sz; i++)
    {
        vec_push(&lookup->nodes_, NULL);
    }

    /* create lookup */
    int offset, found = 0;
    int n = (int) lookup->n_;
    int r = (int) lookup->r_;

    for (int c = 0; c < n; c++)
    {
        offset = -c;
        for (int i = 0; i < n; i++)
        {
            if ((i - offset) % n < r)
                found = 1;

            if (found)
            {
                for (int p = 0; p < r; p++)
                {
                    if (vec_get(lookup->nodes_, i*r + p) == NULL)
                    {
                        lookup->nodes_->data[i*r + p] = vec_get(nodes, c);
                        found = 0;
                        break;
                    }
                }
                if (found)
                    offset++;
            }
        }
    }

    for (int i = 0; i < n; i++)
    {
        ti_node_t * node;
        uint64_t mask = 0;
        for (int p = 0; p < r; p++)
        {
            node = (ti_node_t *) vec_get(lookup->nodes_, i*r + p);
            mask += 1 << node->id;
        }
        lookup->mask_[i] = mask;
    }
}
