/*
 * ti/watch.t.h
 *
 * Watch should be used as an object with at most 2 references. If no stream
 * attached, watch should be cleared and not in use at more than one place.
 */
#ifndef TI_WATCH_T_H_
#define TI_WATCH_T_H_

typedef struct ti_watch_s ti_watch_t;

#include <ti/stream.t.h>

struct ti_watch_s
{
    ti_stream_t * stream;       /* weak reference */
};

#endif  /* TI_WATCH_T_H_ */
