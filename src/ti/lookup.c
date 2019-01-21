/*
 * ti/lookup.c
 */
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <ti/lookup.h>
#include <ti.h>

static uint8_t lookup__cache[64];
static uint8_t lookup__tmp[63];

static void lookup__calculate(ti_lookup_t * lookup);

ti_lookup_t * ti_lookup_create(uint8_t n, uint8_t r)
{
    assert (n && n <= 64);

    ti_lookup_t * lookup = malloc(sizeof(ti_lookup_t));
    if (!lookup)
        return NULL;

    lookup->n = n;
    lookup->r = (n < r) ? n : r;

    lookup->factorial_ = 1;
    for(size_t i = 1; i <= n; ++i)
        lookup->factorial_ *= i;

    lookup__calculate(lookup);

    return lookup;
}

void ti_lookup_destroy(ti_lookup_t * lookup)
{
    if (!lookup)
        return;
    free(lookup);
}

/* TODO: need testing */
_Bool ti_lookup_id_is_ordered(
        ti_lookup_t * lookup,
        uint8_t a,
        uint8_t b,
        uint64_t u)
{
    uint8_t i, n = lookup->n;
    size_t m, f = lookup->factorial_;

    assert (a < b);
    assert (b < lookup->n);

    for (i = 0; i < n; ++i)
        lookup__cache[i] = i;

    for (m = u % f; i > 1; m %= f, --i)
    {
        f /= i;
        lookup__tmp[n-i] = (uint8_t) (m / f);
    }

    for (i = 0; true; ++i)
    {
        uint8_t p = lookup__tmp[i] + i;
        n = lookup__cache[p];

        if (n == a)
            return true;

        if (n == b)
            return false;

        lookup__cache[p] = lookup__cache[i];
        lookup__cache[i] = n;
    }

    assert (0);
    return 0;
}

static void lookup__calculate(ti_lookup_t * lookup)
{
    /* create lookup */
    int nodes, shift, k, m;
    int r = (int) lookup->r;
    int n = (int) lookup->n;
    const int total = LOOKUP_SIZE * r;
    uint8_t * snode, * tnode, target;
    uint64_t * maskp;


    /* set initial lookup */
    for (int i = 0; i < LOOKUP_SIZE; ++i)
        lookup->masks_[i] = (1 << r) -1;

    for (int i = 0; i < n; ++i)
        lookup__cache[i] = i < r ? LOOKUP_SIZE : 0;

    for (int c = r; c < n; ++c)
    {
        nodes = c + 1;
        target = (uint8_t) (total / nodes);
        m = nodes - total % nodes;
        tnode = lookup__cache + c;
        for (int i = 0; i < LOOKUP_SIZE; ++i)
        {
            maskp = lookup->masks_ + i;
            for (k = 0, shift = 1, snode = lookup__cache;
                 k < c;
                 ++k, shift <<= 1, ++snode)
            {
                if ((*maskp & shift) && (*snode > target))
                {
                    --(*snode);
                    if (++(*tnode) > target)
                    {
                        nodes = 0;
                    }
                    else if (*snode == target && (!--m))
                    {
                        ++target;
                    }
                    *maskp &= ~shift;
                    *maskp |= 1 << c;
                    break;
                }
            }
            if (!nodes)
                break;
        }
    }
}
