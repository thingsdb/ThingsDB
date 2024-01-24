/*
 * ti/async.h
 */
#ifndef TI_ASYNC_H_
#define TI_ASYNC_H_

#include <inttypes.h>
#include <ti/future.t.h>
#include <ti/module.h>

ti_module_t * ti_async_get_module(void);
void ti_async_init(void);

#endif  /* TI_EXT_ASYNC_H_ */
