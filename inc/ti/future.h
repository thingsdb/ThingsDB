/*
 * ti/future.h
 */
#ifndef TI_FUTURE_H_
#define TI_FUTURE_H_

#include <inttypes.h>
#include <ti/future.t.h>
#include <ti/query.t.h>

ti_future_t * ti_future_create(
        ti_query_t * query,
        ti_module_t * module,
        size_t nargs,
        uint8_t deep);
void ti_future_destroy(ti_future_t * future);
void ti_future_cancel(ti_future_t * future);
void ti_future_stop(ti_future_t * future);

#endif  /* TI_FUTURE_H_ */
