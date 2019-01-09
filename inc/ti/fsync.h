/*
 * ti/fsync.h
 */
#ifndef TI_FSYNC_H_
#define TI_FSYNC_H_

#include <ti/stream.h>
#include <ti/pkg.h>
#include <ti/ex.h>

int ti_fsync_start(ti_stream_t * stream);
ti_pkg_t * ti_fsync_on_multipart(ti_pkg_t * pkg, ex_t * e);

#endif  /* TI_FSYNC_H_ */
