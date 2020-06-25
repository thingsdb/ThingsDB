/*
 * ti/watch.h
 *
 * Watch should be used as an object with at most 2 references. If no stream
 * attached, watch should be cleared and not in use at more than one place.
 */
#ifndef TI_WATCH_H_
#define TI_WATCH_H_

#include <inttypes.h>
#include <ti/rpkg.t.h>
#include <ti/stream.t.h>
#include <ti/watch.t.h>

ti_watch_t * ti_watch_create(ti_stream_t * stream);
void ti_watch_drop(ti_watch_t * watch);
ti_pkg_t * ti_watch_pkg(
        uint64_t thing_id,
        uint64_t event_id,
        const unsigned char * mpjobs,
        size_t size);
ti_rpkg_t * ti_watch_rpkg(
        uint64_t thing_id,
        uint64_t event_id,
        const unsigned char * jobs,
        size_t size);

#endif  /* TI_WATCH_H_ */
