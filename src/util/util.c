/*
 * util/util.c
 */
#define _GNU_SOURCE
#include <linux/random.h>
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
uint64_t util_now_tsec(void)
{
    (void) clock_gettime(CLOCK_REALTIME, &util__now);
    return util__now.tv_sec;
}

void util_get_random(void * buf, size_t n)
{
    if (syscall(SYS_getrandom, buf, n, GRND_NONBLOCK) != (ssize_t) n)
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
