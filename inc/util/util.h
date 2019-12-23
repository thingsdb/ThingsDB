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
uint64_t util_now_tsec(void);
void util_random_key(char * buf, size_t n);
static inline double util_time_diff(util_time_t * start, util_time_t * end);


/* Returns time difference in seconds */
static inline double util_time_diff(util_time_t * start, util_time_t * end)
{
    return (end->tv_sec - start->tv_sec) +
            (end->tv_nsec - start->tv_nsec) / 1000000000.0;
}

#if RAND_MAX >= 2147483647
#define util_randu64(__x) \
do { \
*(__x) = 0; \
for (int __i = 0; __i < 64; __i += 16) { \
    uint16_t __t = (uint16_t) rand(); \
    *(__x) <<= __i; \
    *(__x) |= __t; \
}} while(0)

#else
#define util_randu64(__x) \
*(__x) = 0; \
for (int __i = 0; __i < 64; __i += 8) { \
    uint8_t __t = (uint8_t) rand(); \
    *(__x) <<= __i; \
    *(__x) |= __t; \
}

#endif


#endif  /* UTIL_H_ */
