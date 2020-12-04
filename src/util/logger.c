/*
 * logger.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <util/logger.h>
#include <stdbool.h>
#include <string.h>

logger_t Logger = {
        .level=10,
        .level_name=NULL,
        .ostream=NULL,
        .flags=0
};

#define LOGGER_CHR_MAP "DIWECU"

#define KNRM  "\x1B[0m"     /* normal */
#define KRED  "\x1B[31m"    /* error */
#define KGRN  "\x1B[32m"    /* info */
#define KYEL  "\x1B[33m"    /* warning */
#define KBLU  "\x1B[34m"    /* -- not used -- */
#define KMAG  "\x1B[35m"    /* critical */
#define KCYN  "\x1B[36m"    /* debug */
#define KWHT  "\x1B[37m"    /* -- not used -- */

const char * LOGGER_LEVEL_NAMES[LOGGER_NUM_LEVELS] =
    {"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"};

const char * LOGGER_COLOR_MAP[LOGGER_NUM_LEVELS] =
    {KCYN, KGRN, KYEL, KRED, KMAG};

#define LOGGER_LOG_STUFF(LEVEL)                                 \
{                                                               \
    time_t t = time(NULL);                                      \
    struct tm tm;                                               \
    gmtime_r(&t, &tm);                                          \
    if (Logger.flags & LOGGER_FLAG_COLORED)                     \
    {                                                           \
        fprintf(Logger.ostream,                                 \
            "%s[%c %d-%0*d-%0*d %0*d:%0*d:%0*d]" KNRM " ",      \
            LOGGER_COLOR_MAP[LEVEL],                            \
            LOGGER_CHR_MAP[LEVEL],                              \
            tm.tm_year + 1900,                                  \
            2, tm.tm_mon + 1,                                   \
            2, tm.tm_mday,                                      \
            2, tm.tm_hour,                                      \
            2, tm.tm_min,                                       \
            2, tm.tm_sec);                                      \
    }                                                           \
    else                                                        \
    {                                                           \
        fprintf(Logger.ostream,                                 \
        "[%c %d-%0*d-%0*d %0*d:%0*d:%0*d] ",                    \
            LOGGER_CHR_MAP[LEVEL],                              \
            tm.tm_year + 1900,                                  \
            2, tm.tm_mon + 1,                                   \
            2, tm.tm_mday,                                      \
            2, tm.tm_hour,                                      \
            2, tm.tm_min,                                       \
            2, tm.tm_sec);                                      \
    }                                                           \
    /* print the actual log line */                             \
    va_list args;                                               \
    va_start(args, fmt);                                        \
    vfprintf(Logger.ostream, fmt, args);                        \
    va_end(args);                                               \
    /* write end of line and flush the stream */                \
    fputc('\n', Logger.ostream);                                \
    fflush(Logger.ostream);                                     \
}

/*
 * Initialize the logger.
 */
void logger_init(struct _LOGGER_IO_FILE * ostream, int log_level)
{
    Logger.ostream = ostream;
    logger_set_level(log_level);
}

/*
 * Set the logger to a given level. (name will be set too)
 */
void logger_set_level(int log_level)
{
    Logger.level = log_level;
    Logger.level_name = LOGGER_LEVEL_NAMES[log_level];
}

/*
 * Returns a log level name for a given log level.
 */
const char * logger_level_name(int log_level)
{
    return LOGGER_LEVEL_NAMES[log_level];
}

void log_with_level(int log_level, const char * fmt, ...)
{
    LOGGER_LOG_STUFF(log_level)
}

void log__debug(const char * fmt, ...)
    LOGGER_LOG_STUFF(LOGGER_DEBUG)

void log__info(const char * fmt, ...)
    LOGGER_LOG_STUFF(LOGGER_INFO)

void log__warning(const char * fmt, ...)
    LOGGER_LOG_STUFF(LOGGER_WARNING)

void log__error(const char * fmt, ...)
    LOGGER_LOG_STUFF(LOGGER_ERROR)

void log__critical(const char * fmt, ...)
    LOGGER_LOG_STUFF(LOGGER_CRITICAL)


char * log_strerror(int errnum, char * buf, size_t n)
{
    memset(buf, 0, n);
    strerror_r(errnum, buf, n);
    if (*buf == '\0')
        strncpy(buf, "unknown errno", n-1);
    return buf;
}

