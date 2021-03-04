/*
 * ti/store/timers.h
 */
#ifndef TI_STORE_TIMERS_H_
#define TI_STORE_TIMERS_H_

#include <util/vec.h>
#include <ti/collection.h>

int ti_store_timers_store(vec_t * timers, const char * fn);
int ti_store_timers_restore(
        vec_t ** timers,
        const char * fn,
        ti_collection_t * collection);

#endif /* TI_STORE_TIMERS_H_ */
