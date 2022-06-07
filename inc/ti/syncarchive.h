/*
 * ti/syncarchive.h
 */
#ifndef TI_SYNCARCHIVE_H_
#define TI_SYNCARCHIVE_H_

#include <ex.h>
#include <ti/stream.h>
#include <ti/pkg.h>

int ti_syncarchive_init(ti_stream_t * stream, uint64_t change_id);
ti_pkg_t * ti_syncarchive_on_part(ti_pkg_t * pkg, ex_t * e);

#endif  /* TI_SYNCARCHIVE_H_ */
