/*
 * util/util.h
 */
#ifndef UTIL_H_
#define UTIL_H_


#include <unistd.h>

static inline int util_sleep(long int ms);


/* sleep in milliseconds */
static inline int util_sleep(long int ms)
{
    return nanosleep((const struct timespec[]){{0, ms * 1000000L}}, NULL);
}

#endif  /* UTIL_H_ */
