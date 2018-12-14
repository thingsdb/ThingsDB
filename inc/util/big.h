/*
 * big.h
 */
#ifndef BIG_H_
#define BIG_H_

typedef struct big_s big_t;

#include <inttypes.h>
#include <stddef.h>

int big_to_str16n(const big_t * big, char * tostr, size_t size);
big_t * big_mulii(const int64_t a, const int64_t b);

static inline _Bool big_is_positive(const big_t * big);
static inline _Bool big_is_negative(const big_t * big);
static inline size_t big_str16_msize(const big_t * big);

struct big_s
{
    uint16_t negative_  : 1;
    uint16_t _pad_      : 15;
    uint16_t n_;                    /* number of parts */
    uint32_t parts_[];
};

static inline _Bool big_is_positive(const big_t * big)
{
    return !big->negative_;
}

static inline _Bool big_is_negative(const big_t * big)
{
    return big->negative_;
}

static inline size_t big_str16_msize(const big_t * big)
{
    return big->n_ * 8 + 1;
}

#endif  /* BIG_H_ */
