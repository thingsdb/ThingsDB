/*
 * big.c
 */
#include <assert.h>
#include <util/big.h>
#include <stdlib.h>
#include <limits.h>


big_t * big_mulii(int64_t a, int64_t b)
{
    assert (a >= -LLONG_MAX && a <= LLONG_MAX);
    assert (b >= -LLONG_MAX && b <= LLONG_MAX);
    uint64_t ua, ub, na, nb, part;
    big_t * big = malloc(sizeof(big_t) + sizeof(uint32_t) * 4);
    if (!big)
        return NULL;

    big->negative_ = (a<0)^(b<0);

    ua = (uint64_t) llabs(a);
    ub = (uint64_t) llabs(b);

    na = ua & 0xffff;
    nb = ub & 0xffff;

    part = na * nb;

    big->parts_[0] = part & 0xffff;

    return big;
}
