/*
 * ti/async.h
 */
#ifndef TI_ASYNC_H_
#define TI_ASYNC_H_

#include <inttypes.h>
#include <ti/future.t.h>
#include <ti/module.h>

void ti_async_cb(ti_future_t * future);
ti_module_t * ti_async_get_module(void);

#endif  /* TI_EXT_ASYNC_H_ */
