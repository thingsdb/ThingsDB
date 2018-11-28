/*
 * util/util.h
 */
#ifndef UTIL_H_
#define UTIL_H_


#include <sys/time.h>


static inline double util_time_diff(struct timespec start, struct timespec end);


/* Returns time difference in seconds */
static inline double util_time_diff(struct timespec start, struct timespec end)
{
    return (end.tv_sec - start->tv_sec) +
            (end.tv_nsec - start->tv_nsec) / 1000000000.0f;
}

#endif  /* UTIL_H_ */
