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
    uint64_t ua, ub, part, carry;
    uint64_t a0, a1, b0, b1;
    big_t * big = calloc(1, sizeof(big_t) + sizeof(uint32_t) * nparts);
    if (!big)
        return NULL;

    big->negative_ = (a<0)^(b<0);
    big->n_ = nparts;

    ua = (uint64_t) llabs(a);
    ub = (uint64_t) llabs(b);

    a0 = ua & 0xffffffff;
    a1 = (ua & 0xffffffff00000000) >> 32;
    b0 = ub & 0xffffffff;
    b1 = (ub & 0xffffffff00000000) >> 32;

    printf("a0: %lu\n", a0);
    printf("a1: %lu\n", a1);
    printf("b0: %lu\n", b0);
    printf("b1: %lu\n", b1);


    part = a0 * b0;
    big->parts_[3] = part & 0xffffffff;
    big->parts_[2] = (part & 0xffffffff00000000) >> 32;

    part = a1*b0 + big->parts_[2];;
    big->parts_[2] = part & 0xffffffff;
    big->parts_[1] = carry = (part & 0xffffffff00000000) >> 32;

    part = a0*b1 + big->parts_[2];
    big->parts_[2] = part & 0xffffffff;

    carry += (part & 0xffffffff00000000) >> 32;
    big->parts_[1] = carry & 0xffffffff;
    big->parts_[0] = carry & 0xffffffff00000000;

    printf("part0: %u\n", big->parts_[0]);


    part = a1*b1 + big->parts_[1];
    big->parts_[1] = part & 0xffffffff;
    big->parts_[0] = (part & 0xffffffff00000000) >> 32;

    for (int i = 0; i < 4; ++i)
    {
        printf("part %d: %u\n", i, big->parts_[i]);
    }

    return big;
}



