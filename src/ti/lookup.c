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

    ti_lookup_t * lookup = malloc(sizeof(ti_lookup_t) + n * sizeof(uint64_t));
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
    const uint8_t empty = 0xff;
    int offset, found = 0;
    int r = (int) lookup->r;
    int n = (int) lookup->n;

    /* set lookup to -1 */
    for (int i = 0; i < n; i++)
        lookup__cache[i] = empty;

    for (int c = 0; c < n; c++)
    {
        offset = -c;
        for (int i = 0; i < n; i++)
        {
            if ((i - offset) % n < r)
                found = 1;

            if (!found)
                continue;

            for (int p = 0; p < r; p++)
            {
                if (lookup__cache[i*r + p] == empty)
                {
                    lookup__cache[i*r + p] = (uint8_t) c;
                    found = 0;
                    break;
                }
            }

            if (found)
                offset++;
        }
    }

    for (int i = 0; i < n; i++)
    {
        uint8_t node_id;
        uint64_t mask = 0;

        for (int p = 0; p < r; p++)
        {
            node_id = lookup__cache[i*r + p];
            mask += 1 << node_id;
        }

        lookup->mask_[i] = mask;
    }
}
