/*
 * ti/future.h
 */
#ifndef TI_FUTURE_H_
#define TI_FUTURE_H_

#include <inttypes.h>
#include <ti/ext.h>
#include <ti/future.t.h>
#include <ti/query.t.h>

ti_future_t * ti_future_create(
        ti_query_t * query,
        ti_ext_cb ext_cb,
        size_t nargs);
void ti_future_destroy(ti_future_t * future);


#endif  /* TI_FUTURE_H_ */
