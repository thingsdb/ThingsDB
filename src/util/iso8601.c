/*
 * util/iso8601.c - parse ISO 8601 dates.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <util/iso8601.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <ctype.h>
#include <inttypes.h>
#include <assert.h>

#define TZ_DIGITS \
    for (len = 0; *pt && isdigit(*pt); pt++, len++);

#define TZ_RESULT(tz_fmt, check_min) \
    if (*pt == 0)                                               \
        return iso8601__ts(str, tz_fmt, 0);                     \
                                                                \
    if (*pt == 'Z' && *(pt + 1) == 0)                           \
        return iso8601__ts(str, tz_fmt "Z", 0);                 \
                                                                \
    if (*pt == '+' || (check_min && *pt == '-' && (sign = 1)))  \
    {                                                           \
        offset = 0;                                             \
                                                                \
        pt++;                                                   \
        if (!isdigit(*pt))                                      \
            return -1;                                          \
        offset += (*pt - '0') * 36000;                          \
                                                                \
        pt++;                                                   \
        if (!isdigit(*pt))                                      \
            return -1;                                          \
        offset += (*pt - '0') * 3600;                           \
                                                                \
        pt++;                                                   \
        if (*pt == ':')                                         \
            pt++;                                               \
                                                                \
        if (*pt == 0)                                           \
            return iso8601__ts(str, tz_fmt, offset * sign);     \
                                                                \
        if (!isdigit(*pt))                                      \
            return -1;                                          \
        offset += (*pt - '0') * 600;                            \
                                                                \
        pt++;                                                   \
        if (!isdigit(*pt))                                      \
            return -1;                                          \
        offset += (*pt - '0') * 60;                             \
                                                                \
        pt++;                                                   \
        if (*pt != 0)                                           \
            return -1;                                          \
                                                                \
        return iso8601__ts(str, tz_fmt, offset * sign);         \
    }

static int64_t iso8601__ts(
        const char * str,
        const char * fmt,
        int64_t offset)
{
    struct tm tm;
    int64_t ts;

    memset(&tm, 0, sizeof(struct tm));

    /* month day should default to 1, not 0 */
    if (!tm.tm_mday)
        tm.tm_mday = 1;

    if (strptime(str, fmt, &tm) == NULL)
        /* parsing date string failed */
        return -1;

    ts = (int64_t) mktime(&tm);

    return ts + offset;
}

int64_t iso8601_parse_date(const char * str)
{
    size_t len;
    const char * pt = str;
    int64_t offset;
    int sign = -1;
    int use_t = 0;

    /* read year */
    TZ_DIGITS
    if (len != 4)
        return -1;

    TZ_RESULT("%Y", 0)
    if (*pt != '-')
        return -1;
    pt++;

    /* read month */
    TZ_DIGITS
    if (!len || len > 2)
        return -1;

    TZ_RESULT("%Y-%m", 0)
    if (*pt != '-')
        return -1;
    pt++;

    /* read day */
    TZ_DIGITS
    if (!len || len > 2)
        return -1;

    TZ_RESULT("%Y-%m-%d", 1)
    if (*pt != ' ' && (use_t = 1) && *pt != 'T')
        return -1;
    pt++;

    /* read hours */
    for (len = 0; *pt && isdigit(*pt); pt++, len++);

    if (!len || len > 2)
        return -1;

    TZ_RESULT((use_t) ? "%Y-%m-%dT%H" : "%Y-%m-%d %H", 1)
    if (*pt != ':')
        return -1;

    pt++;

    /* read minutes */
    TZ_DIGITS
    if (!len || len > 2)
        return -1;

    TZ_RESULT((use_t) ? "%Y-%m-%dT%H:%M" : "%Y-%m-%d %H:%M", 1)
    if (*pt != ':')
        return -1;
    pt++;

    /* read seconds */
    TZ_DIGITS
    if (!len || len > 2)
        return -1;

    TZ_RESULT((use_t) ? "%Y-%m-%dT%H:%M:%S" : "%Y-%m-%d %H:%M:%S", 1)

    return -1;
}

int64_t iso8601_parse_date_n(const char * str, size_t n)
{
    static const size_t max_n = strlen("2000-00-00 00:00:00+01.00Z") + 1;
    char buf[max_n];

    if (n >= max_n)
        return -1;

    memcpy(buf, str, n);
    buf[n] = '\0';

    return iso8601_parse_date(buf);
}
