/*
 * logger.h
 */
#ifndef LOGGER_H_
#define LOGGER_H_

#ifdef __APPLE__
#define _LOGGER_IO_FILE __sFILE
#else
#define _LOGGER_IO_FILE _IO_FILE
#endif

#include <stddef.h>

char * log_strerror(int errnum, char * buf, size_t n);

#define log_errno_file(__s, __err, __fn) \
do { \
    char __ebuf[512]; \
    log_error( \
        __s" `%s` (%s)", \
        __fn, log_strerror(__err, __ebuf, sizeof(__ebuf)) \
    ); \
} while(0)

#define LOGGER_DEBUG 0
#define LOGGER_INFO 1
#define LOGGER_WARNING 2
#define LOGGER_ERROR 3
#define LOGGER_CRITICAL 4

#define LOGGER_NUM_LEVELS 5

#define LOGGER_FLAG_COLORED 1

typedef struct logger_s logger_t;

#include <stdio.h>
#include <uv.h>

const char * LOGGER_LEVEL_NAMES[LOGGER_NUM_LEVELS];

void logger_init(struct _LOGGER_IO_FILE * ostream, int log_level);
void logger_set_level(int log_level);
const char * logger_level_name(int log_level);

void log_with_level(int log_level, const char * fmt, ...);
void log__debug(const char * fmt, ...);
void log__info(const char * fmt, ...);
void log__warning(const char * fmt, ...);
void log__error(const char * fmt, ...);
void log__critical(const char * fmt, ...);

extern logger_t Logger;

#define log_debug(fmt, ...)                         \
    do if (Logger.level == LOGGER_DEBUG) {          \
        log__debug(fmt, ##__VA_ARGS__);             \
    } while(0)

#define log_info(fmt, ...)                          \
    do if (Logger.level <= LOGGER_INFO) {           \
        log__info(fmt, ##__VA_ARGS__);              \
    } while(0)

#define log_warning(fmt, ...)                       \
    do if (Logger.level <= LOGGER_WARNING) {        \
        log__warning(fmt, ##__VA_ARGS__);           \
    } while(0)

#define log_error(fmt, ...)                         \
    do if (Logger.level <= LOGGER_ERROR) {          \
        log__error(fmt, ##__VA_ARGS__);             \
    } while(0)

#define log_critical(fmt, ...)                                  \
    do if (Logger.level <= LOGGER_CRITICAL) {                   \
        fprintf(Logger.ostream, "%s:%d ", __FILE__, __LINE__);  \
        log__critical(fmt, ##__VA_ARGS__);                      \
    } while(0)

#define LOGC(fmt, ...)                                          \
    do {                                                        \
        log_critical(fmt, ##__VA_ARGS__);                       \
    } while(0)

#ifndef NDEBUG
#define assert_log(x, fmt, ...) \
    do if (!(x)) LOGC(fmt, ##__VA_ARGS__); while(0)
#else
#define assert_log(x, fmt, ...) (void) ((const char *) 0)
#endif

struct logger_s
{
    struct _LOGGER_IO_FILE * ostream;
    int level;
    const char * level_name;
    int flags;
};

#endif /* LOGGER_H_ */
