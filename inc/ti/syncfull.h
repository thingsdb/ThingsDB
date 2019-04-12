/*
 * ti/syncfull.h
 */
#ifndef TI_SYNCFULL_H_
#define TI_SYNCFULL_H_

#include <ti/stream.h>
#include <ti/pkg.h>
#include <ti/ex.h>

int ti_syncfull_start(ti_stream_t * stream);
ti_pkg_t * ti_syncfull_on_part(ti_pkg_t * pkg, ex_t * e);

#endif  /* TI_FSYNC_H_ */
