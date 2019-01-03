/*
 * ti/watch.h
 */
#ifndef TI_WATCH_H_
#define TI_WATCH_H_

typedef struct ti_watch_s ti_watch_t;

#define TI_WATCH_T                                                          \
{                                                                           \
        ti_stream_t * stream;       /* weak reference */

#include <ti/pkg.h>
#include <ti/ex.h>
#include <ti/user.h>
#include <ti/stream.h>
#include <util/logger.h>

ti_watch_t * ti_watch_create(ti_stream_t * stream);
static inline void ti_watch_stop(ti_watch_t * watch);
static inline void ti_watch_free(ti_watch_t * watch);

struct ti_watch_s
    TI_WATCH_T
};

static inline void ti_watch_stop(ti_watch_t * watch)
{
    watch->stream = NULL;
}

static inline void ti_watch_free(ti_watch_t * watch)
{
    free(watch);
}

#endif  /* TI_WATCH_H_ */
