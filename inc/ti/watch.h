/*
 * ti/watch.h
 */
#ifndef TI_WATCH_H_
#define TI_WATCH_H_

typedef struct ti_watch_s ti_watch_t;

#include <ti/pkg.h>
#include <ti/ex.h>
#include <ti/user.h>
#include <ti/stream.h>
#include <util/logger.h>

ti_watch_t * ti_watch_create(ti_stream_t * stream);
static inline void ti_watch_stop(ti_watch_t * watch);
static inline void ti_watch_free(ti_watch_t * watch);
int ti_watch_req_watch(ti_pkg_t * pkg, ti_user_t * user, ex_t * e);

struct ti_watch_s
{
    ti_stream_t * stream;       /* weak reference */
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
