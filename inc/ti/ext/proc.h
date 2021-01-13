/*
 * ti/ext/proc.h
 */
#ifndef TI_EXT_PROC_H_
#define TI_EXT_PROC_H_

#include <inttypes.h>
#include <ti/future.t.h>

int ti_ext_proc_init(void);
//void ti_ext_proc_destroy(void);
void ti_ext_proc_cb(ti_future_t * future);

#endif  /* TI_EXT_PROC_H_ */
