/*
 * util/big.h
 */
#ifndef BIG_H_
#define BIG_H_

typedef struct big_s big_t;

#include <inttypes.h>
#include <stddef.h>


int big_to_str16n(const big_t * big, char * tostr, size_t size);
big_t * big_from_str(const char * str, int base);
big_t * big_from_str2n(const char * str, size_t n);
big_t * big_null(void);
big_t * big_from_int64(const int64_t i);
int64_t big_to_int64(const big_t * big);
big_t * big_from_double(const double d);
double big_to_double(const big_t * big);
big_t * big_mulii(const int64_t a, const int64_t b);
big_t * big_mulbb(const big_t * a, const big_t * b);
big_t * big_mulbi(const big_t * a, const int64_t b);
double big_mulbd(const big_t * a, const double b);

static inline _Bool big_is_positive(const big_t * big);
static inline _Bool big_is_negative(const big_t * big);
static inline size_t big_str16_msize(const big_t * big);
static inline _Bool big_fits_int64(const big_t * big);
static inline _Bool big_is_null(const big_t * big);
static inline void big_make_negative(big_t * big);
static inline void big_make_positive(big_t * big);

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
    return big->n_ ? big->n_ * 8 + 1 : 2;
}

static inline _Bool big_fits_int64(const big_t * big)
{
    return (
        big->n_ < 2 || (big->n_ == 2 && (
            (~big->parts_[0] & 0x80000000UL) || (
                big->negative_ &&
                big->parts_[0] == 0x80000000UL &&
                big->parts_[1] == 0
            )
        ))
    );
}

static inline _Bool big_is_null(const big_t * big)
{
    return big->n_ == 0;
}

static inline void big_make_negative(big_t * big)
{
    big->negative_ = 1;
}

static inline void big_make_positive(big_t * big)
{
    big->negative_ = 0;
}

#endif  /* BIG_H_ */
