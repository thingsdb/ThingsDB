/*
 * util/util.h
 */
#ifndef UTIL_H_
#define UTIL_H_

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

#endif  /* UTIL_H_ */
