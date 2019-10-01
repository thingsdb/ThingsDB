/*
 * ti/watch.h
 *
 * Watch should be used as an object with at most 2 references. If no stream
 * attached, watch should be cleared and not in use at more than one place.
 */
#ifndef TI_WATCH_H_
#define TI_WATCH_H_

typedef struct ti_watch_s ti_watch_t;

#include <ti/pkg.h>
#include <ex.h>
#include <ti/user.h>
#include <ti/stream.h>
#include <util/logger.h>

ti_watch_t * ti_watch_create(ti_stream_t * stream);
void ti_watch_drop(ti_watch_t * watch);
ti_pkg_t * ti_watch_pkg(
        uint64_t thing_id,
        uint64_t event_id,
        const unsigned char * jobs,
        size_t n);

struct ti_watch_s
{
    ti_stream_t * stream;       /* weak reference */
};

#endif  /* TI_WATCH_H_ */
