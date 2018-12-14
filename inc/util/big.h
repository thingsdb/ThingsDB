/*
 * big.h
 */
#ifndef BIG_H_
#define BIG_H_

typedef struct big_s big_t;

#include <inttypes.h>

big_t * big_mulii(int64_t a, int64_t b);

struct big_s
{
    uint32_t negative_  : 1;
    uint32_t size_      : 31;
    uint32_t parts_[];
};

#endif  /* BIG_H_ */
