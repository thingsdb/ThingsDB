/*
 * ti/syncevents.h
 */
#ifndef TI_SYNCEVENTS_H_
#define TI_SYNCEVENTS_H_

#include <ti/stream.h>
#include <ti/pkg.h>
#include <ti/ex.h>

int ti_syncevents_init(ti_stream_t * stream, uint64_t event_id);
ti_pkg_t * ti_syncevents_on_part(ti_pkg_t * pkg, ex_t * e);
int ti_syncevents_done(ti_stream_t * stream);

#endif  /* TI_SYNCEVENTS_H_ */
