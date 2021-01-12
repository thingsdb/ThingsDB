/*
 * ti/ext/async.h
 */
#ifndef TI_EXT_ASYNC_H_
#define TI_EXT_ASYNC_H_

#include <inttypes.h>
#include <ti/future.t.h>
#include <ti/ext.h>

void ti_ext_async_cb(ti_future_t * future);
ti_ext_t * ti_ext_async_get(void);

#endif  /* TI_EXT_ASYNC_H_ */
