/*
 * ti/rjob.h
 */
#ifndef TI_RJOB_H_
#define TI_RJOB_H_

#include <util/mpack.h>
#include <ti/event.h>

int ti_rjob_run(ti_event_t * ev,  mp_unp_t * up);

#endif  /* TI_RJOB_H_ */
