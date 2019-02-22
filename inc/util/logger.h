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

int logger_init(struct _LOGGER_IO_FILE * ostream, int log_level);
void logger_destroy(void);
void logger_set_level(int log_level);
const char * logger_level_name(int log_level);

void log__debug(const char * fmt, ...);
void log__info(const char * fmt, ...);
void log__warning(const char * fmt, ...);
void log__error(const char * fmt, ...);
void log__critical(const char * fmt, ...);

extern logger_t Logger;

#define log_debug(fmt, ...)                         \
    do if (Logger.level == LOGGER_DEBUG) {          \
        uv_mutex_lock(&Logger.lock);                \
        log__debug(fmt, ##__VA_ARGS__);             \
        uv_mutex_unlock(&Logger.lock);              \
    } while(0)

#define log_info(fmt, ...)                          \
    do if (Logger.level <= LOGGER_INFO) {           \
        uv_mutex_lock(&Logger.lock);                \
        log__info(fmt, ##__VA_ARGS__);              \
        uv_mutex_unlock(&Logger.lock);              \
    } while(0)

#define log_warning(fmt, ...)                       \
    do if (Logger.level <= LOGGER_WARNING) {        \
        uv_mutex_lock(&Logger.lock);                \
        log__warning(fmt, ##__VA_ARGS__);           \
        uv_mutex_unlock(&Logger.lock);              \
    } while(0)

#define log_error(fmt, ...)                         \
    do if (Logger.level <= LOGGER_ERROR) {          \
        uv_mutex_lock(&Logger.lock);                \
        log__error(fmt, ##__VA_ARGS__);             \
        uv_mutex_unlock(&Logger.lock);              \
    } while(0)

#define log_critical(fmt, ...)                      \
    do if (Logger.level <= LOGGER_CRITICAL) {       \
        uv_mutex_lock(&Logger.lock);                \
        log__critical(fmt, ##__VA_ARGS__);          \
        uv_mutex_unlock(&Logger.lock);              \
    } while(0)

#define LOGC(fmt, ...)                                          \
    do {                                                        \
        fprintf(Logger.ostream, "%s:%d ", __FILE__, __LINE__);  \
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
    uv_mutex_t lock;
    uv_thread_t main_thread;
};

#endif /* LOGGER_H_ */
