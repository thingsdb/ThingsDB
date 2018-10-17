/*
 * lookup.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/lookup.h>

static void ti__lookup_calculate(ti_lookup_t * lookup, const vec_t * nodes);

ti_lookup_t * ti_lookup_create(
        uint8_t n,
        uint8_t redundancy,
        const vec_t * nodes)
{
    assert (n <= nodes->n);
    ti_lookup_t * lookup = (ti_lookup_t *) malloc(
            sizeof(ti_lookup_t) + n * sizeof(uint64_t));
    if (!lookup) return NULL;
    lookup->n_ = n;
    lookup->r_ = (n < redundancy) ? n : redundancy;
    lookup->nodes_ = vec_new(lookup->r_ * n);
    if (!lookup->nodes_)
    {
        ti_lookup_destroy(lookup);
        return NULL;
    }
    ti__lookup_calculate(lookup, nodes);
    return lookup;
}

void ti_lookup_destroy(ti_lookup_t * lookup)
{
    if (!lookup) return;
    free(lookup->nodes_);
    free(lookup);
}

_Bool ti_lookup_node_has_id(
        ti_lookup_t * lookup,
        ti_node_t * node,
        uint64_t id)
{
    return lookup->mask_[id % lookup->n_] & (1 << node->id);
}

static void ti__lookup_calculate(ti_lookup_t * lookup, const vec_t * nodes)
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
            {
                found = 1;
            }
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
                {
                    offset++;
                }
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
