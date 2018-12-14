/*
 * big.c
 */
#include <assert.h>
#include <util/big.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>


int big_to_str16n(const big_t * big, char * tostr, size_t size)
{
    ssize_t sz = (ssize_t) size;
    int count;
    uint16_t n = 0;
    PRIu64;

    while (n < big->n_ && big->parts_[n] == 0)
        ++n;

    if (n == big->n_)
        return snprintf(tostr, size, "0");

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


big_t * big_mulii(const int64_t a, const int64_t b)
{
    assert (a >= -LLONG_MAX && a <= LLONG_MAX);
    assert (b >= -LLONG_MAX && b <= LLONG_MAX);

    uint16_t nparts = 4;
    uint64_t carry = 0;
    uint64_t ua, ub, na, nb, part;
    big_t * big = malloc(sizeof(big_t) + sizeof(uint32_t) * nparts);
    if (!big)
        return NULL;

    big->negative_ = (a<0)^(b<0);
    big->n_ = nparts;

    ua = (uint64_t) llabs(a);
    ub = (uint64_t) llabs(b);

    na = ua & 0xffff;
    nb = ub & 0xffff;

    part = na * nb;

    while (--nparts)
    {
        big->parts_[nparts] = (carry + part) & 0xffff;
        carry = (part & 0xffff0000) >> 32;
        part = 0;
    }

    return big;
}



