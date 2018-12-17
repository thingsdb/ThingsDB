/*
 * big.c
 */
#include <assert.h>
#include <util/big.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

int big_to_str16n(const big_t * big, char * tostr, size_t size)
{
    ssize_t sz = (ssize_t) size;
    int count;
    uint16_t n = 0;
    PRIu64;

    if (!big->n_)
        return snprintf(tostr, size, "0");

    assert (big->parts_[n]);

    count = snprintf(tostr, size, "%"PRIx32, big->parts_[n]);

    if (count < 0)
        return count;

    sz = count >= sz ? 0 : sz - count;

    for (++n; n < big->n_ ; ++n)
    {
        int rc = snprintf(tostr + count, sz, "%08"PRIx32, big->parts_[n]);

        if (rc > 0)
            count += rc;
        else
            return rc;

        sz = rc >= sz ? 0 : sz - rc;
    }

    return count;
}

big_t * big_null(void)
{
    big_t * big = malloc(sizeof(big_t));
    if (!big)
        return NULL;
    big->n_ = 0;
    big->negative_ = 0;
    return big;
}

big_t * big_from_int64(const int64_t i)
{
    assert (i >= -LLONG_MAX && i <= LLONG_MAX);

    big_t * big;
    uint32_t count, lu;
    uint64_t u = (uint64_t) llabs(i);

    lu = (u & 0xffffffff00000000) >> 32;
    count = lu ? 2 : i ? 1 : 0;
    big = malloc(sizeof(big_t) + sizeof(uint32_t) * count);
    if (!big)
        return NULL;

    big->negative_ = i < 0;
    big->n_ = count;

    if (lu)
    {
        big->parts_[0] = lu;
        big->parts_[1] = u & 0xffffffff;
    }
    else if (i)
    {
        big->parts_[0] = u & 0xffffffff;
    }

    return big;
}

int64_t big_to_int64(const big_t * big)
{
    assert (big_fits_int64(big));
    int64_t i;
    uint64_t u = big->parts_[0];

    u <<= 32;
    u += big->parts_[1];
    i = (int64_t) u;

    return big->negative_ ? -i : i;
}

big_t * big_mulii(const int64_t a, const int64_t b)
{
    assert (a >= -LLONG_MAX && a <= LLONG_MAX);
    assert (b >= -LLONG_MAX && b <= LLONG_MAX);

    big_t * big;
    uint32_t start, count, size, parts[4];
    uint64_t a0, a1, b0, b1, part;

    if (!a || !b)
        return big_null();

    a0 = (uint64_t) llabs(a);
    b0 = (uint64_t) llabs(b);

    a1 = (a0 & 0xffffffff00000000) >> 32;
    b1 = (b0 & 0xffffffff00000000) >> 32;

    a0 &= 0xffffffff;
    b0 &= 0xffffffff;

    part = a0*b0;

    parts[3] = part & 0xffffffff;
    parts[2] = (part & 0xffffffff00000000) >> 32;

    part = a1*b0 + parts[2];;
    parts[2] = part & 0xffffffff;
    parts[1] = (part & 0xffffffff00000000) >> 32;

    part = a0*b1 + parts[2];
    parts[2] = part & 0xffffffff;
    parts[1] += (part & 0xffffffff00000000) >> 32;

    part = a1*b1 + parts[1];
    parts[1] = part & 0xffffffff;
    parts[0] = (part & 0xffffffff00000000) >> 32;

    for (start = 0; start < 4 && !parts[start]; ++start);
    count = (4 - start);
    size = count * sizeof(uint32_t);

    big = malloc(sizeof(big_t) + size);
    if (!big)
        return NULL;

    big->negative_ = (a<0)^(b<0);
    big->n_ = count;
    memcpy(big->parts_, parts + start, size);

    return big;
}

big_t * big_mulbb(const big_t * a, const big_t * b)
{
    big_t * big;
    uint32_t nmax = a->n_ + b->n_;
    uint32_t i = nmax - 1, count, size, parts[nmax];
    uint64_t part;

    if (big_is_null(a) || big_is_null(b))
        return big_null();

    for (count = 0; count < nmax; ++count)
        parts[count] = 0;

    for (uint_fast16_t an = a->n_; an--; --i)
    {
        for (uint_fast16_t bn = b->n_, x = i; bn--; )
        {
            part = (uint64_t) a->parts_[an]*(uint64_t) b->parts_[bn]+parts[x];
            parts[x] = part & 0xffffffff;
            parts[--x] += (part & 0xffffffff00000000) >> 32;
        }
    }

    for (i = 0; i < nmax && !parts[i]; ++i);
    count = (nmax - i);
    size = count * sizeof(uint32_t);

    big = malloc(sizeof(big_t) + size);
    if (!big)
        return NULL;

    big->negative_ = a->negative_^b->negative_;
    big->n_ = count;
    memcpy(big->parts_, parts + i, size);

    return big;
}

big_t * big_mulbi(const big_t * a, const int64_t b)
{
    assert (b >= -LLONG_MAX && b <= LLONG_MAX);

    big_t * big;
    uint32_t nmax = a->n_ + 2;
    uint32_t i = nmax - 1, count, size, parts[nmax];
    uint64_t b0, b1, part;

    if (big_is_null(a) || !b)
        return big_null();

    b0 = (uint64_t) llabs(b);
    b1 = (b0 & 0xffffffff00000000) >> 32;
    b0 &= 0xffffffff;

    for (unsigned int k = 0; k < nmax; ++k)
        parts[k] = 0;

    for (uint_fast16_t an = a->n_; an--; ++i)
    {

        part = (uint64_t) a->parts_[an]*b0 + parts[i];
        parts[i] = part & 0xffffffff;
        parts[--i] += (part & 0xffffffff00000000) >> 32;

        part = (uint64_t) a->parts_[an]*b1 + parts[i];
        parts[i] = part & 0xffffffff;
        parts[--i] += (part & 0xffffffff00000000) >> 32;
    }

    for (i = 0; i < nmax && !parts[i]; ++i);
    count = (nmax - i);
    size = count * sizeof(uint32_t);

    big = malloc(sizeof(big_t) + size);
    if (!big)
        return NULL;

    big->negative_ = a->negative_^(b<0);
    big->n_ = count;
    memcpy(big->parts_, parts + i, size);

    return big;
}

double big_mulbd(const big_t * a, const double b)
{
    double r = 0, d;
    uint_fast16_t i, n, k;
    for (i = 0, n = a->n_; i < n; ++i)
    {
        d = b * (double) a->parts_[i];
        k = n - i;
        while (--k)
            d *= (double) 0x100000000;
        r += d;
    }
    return a->negative_ ? -r : r;
}

