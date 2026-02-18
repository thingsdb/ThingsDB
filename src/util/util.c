/*
 * util/util.c
 */
#define _GNU_SOURCE
#ifndef __APPLE__
#include <linux/random.h>
#endif
#include <stdlib.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
#include <util/logger.h>
#include <util/util.h>

static struct timespec util__now;
static const char util__charset[65] = \
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
        "abcdefghijklmnopqrstuvwxyz" \
        "0123456789+/";
static const int uril__charset_sz = (sizeof(util__charset) - 1);

double util_now(void)
{
    (void) clock_gettime(CLOCK_REALTIME, &util__now);
    return ((double) util__now.tv_sec) + (util__now.tv_nsec / 1000000000.0);
}

/* Returns the current UNIX time-stamp in seconds */
uint64_t util_now_usec(void)
{
    (void) clock_gettime(CLOCK_REALTIME, &util__now);
    return util__now.tv_sec;
}

/* Returns the current UNIX time-stamp in seconds */
time_t util_now_tsec(void)
{
    (void) clock_gettime(CLOCK_REALTIME, &util__now);
    return util__now.tv_sec;
}

void util_get_random(void * buf, size_t n)
{
#ifndef __APPLE__
    if (syscall(SYS_getrandom, buf, n, GRND_NONBLOCK) != (ssize_t) n)
#endif
    {
        log_warning(
                "getrandom(..) has failed; "
                "fall-back using rand() for generating random data");

        unsigned char * c = buf;
        while (n--)
        {
            *c = (unsigned char) (rand() % 255);
            ++c;
        }
    }
}

/*
 * This will generate a random key using getrandom(..) which in turn will use
 * the `urandom` source if possible. If for some reason the getrandom(..)
 * function cannot be used, for example, when not enough random source is
 * available, then rand() is used as a fall-back scenario.
 */
void util_random_key(char * buf, size_t n)
{
    util_get_random(buf, n);

    while (n--)
    {
        unsigned char idx = (unsigned char) buf[n];
        buf[n] = util__charset[idx % uril__charset_sz];
    }
}

void uuid_v7(uint8_t uuid[16])
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    uint64_t ms = (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
    uuid[0] = (ms >> 40) & 0xFF;
    uuid[1] = (ms >> 32) & 0xFF;
    uuid[2] = (ms >> 24) & 0xFF;
    uuid[3] = (ms >> 16) & 0xFF;
    uuid[4] = (ms >> 8) & 0xFF;
    uuid[5] = ms & 0xFF;

#ifndef __APPLE__
    if (syscall(SYS_getrandom, &uuid[6], 10, GRND_NONBLOCK) != 10)
#endif
    {
        unsigned char * c = &uuid[6];
        size_t n = 10;
        while (n--)
        {
            *c = (unsigned char) (rand() % 255);
            ++c;
        }
    }

    uuid[6] = (uuid[6] & 0x0F) | 0x70;
    uuid[8] = (uuid[8] & 0x3F) | 0x80;
}