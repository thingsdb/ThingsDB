/*
 * util/util.h
 */
#ifndef UTIL_H_
#define UTIL_H_

#include <stdlib.h>
#include <sys/time.h>
#include <inttypes.h>
#include <stddef.h>

typedef struct timespec util_time_t;


double util_now(void);
uint64_t util_now_usec(void);
time_t util_now_tsec(void);
void util_get_random(void * buf, size_t n);
void util_random_key(char * buf, size_t n);

/* Returns time difference in seconds */
static inline double util_time_diff(util_time_t * start, util_time_t * end)
{
    return (end->tv_sec - start->tv_sec) +
            (end->tv_nsec - start->tv_nsec) / 1000000000.0;
}

#if RAND_MAX >= 2147483647
#define util_randu64(x__) \
do { \
*(x__) = 0; \
for (int i__ = 0; i__ < 64; i__ += 16) { \
    uint16_t t__ = (uint16_t) rand(); \
    *(x__) <<= i__; \
    *(x__) |= t__; \
}} while(0)

#else
#define util_randu64(x__) \
*(x__) = 0; \
for (int i__ = 0; i__ < 64; i__ += 8) { \
    uint8_t t__ = (uint8_t) rand(); \
    *(x__) <<= i__; \
    *(x__) |= t__; \
}

#endif


#endif  /* UTIL_H_ */
