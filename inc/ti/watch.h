/*
 * ti/watch.h
 *
 * Watch should be used as an object with at most 2 references. If no stream
 * attached, watch should be cleared and not in use at more than one place.
 */
#ifndef TI_WATCH_H_
#define TI_WATCH_H_

#include <stdlib.h>
#include <inttypes.h>
#include <ti/stream.t.h>
#include <ti/watch.t.h>

ti_watch_t * ti_watch_create(ti_stream_t * stream);

static inline void ti_watch_drop(ti_watch_t * watch)
{
    if (watch && watch->stream)
        watch->stream = NULL;
    else
        free(watch);
}

#endif  /* TI_WATCH_H_ */
