/*
 * util/util.c
 */
#include <util/util.h>
#include <stdlib.h>
#include <time.h>
#include <util/logger.h>

static struct timespec util__now;

double util_now(void)
{
    (void) clock_gettime(CLOCK_REALTIME, &util__now);
    return ((double) util__now.tv_sec) + (util__now.tv_nsec / 1000000000.0f);
}

uint64_t util_now_tsec(void)
{
    (void) clock_gettime(CLOCK_REALTIME, &util__now);
    return util__now.tv_sec;
}
