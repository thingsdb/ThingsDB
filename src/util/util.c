/*
 * util/util.c
 */
#include <util/util.h>
#include <stdlib.h>
#include <time.h>
#include <util/logger.h>

static struct timespec util__now;
static const char util__charset[65] = \
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
        "abcdefghijklmnopqrstuvwxyz" \
        "0123456789+/";
static const int uril__charset_sz = (sizeof(util__charset) - 1);

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

void util_random_key(char * buf, size_t n)
{
    int idx;
    while (n--)
    {
        idx = rand() % uril__charset_sz;
        buf[n] = util__charset[idx];
    }
}
