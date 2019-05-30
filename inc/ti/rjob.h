/*
 * ti/rjob.h
 */
#ifndef TI_RJOB_H_
#define TI_RJOB_H_

#include <qpack.h>
#include <ti/event.h>

int ti_rjob_run(ti_event_t * ev, qp_unpacker_t * unp);

#endif  /* TI_RJOB_H_ */
