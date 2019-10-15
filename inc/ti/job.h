/*
 * ti/job.h
 */
#ifndef TI_JOB_H_
#define TI_JOB_H_

#include <util/mpack.h>
#include <ti/collection.h>
#include <ti/thing.h>

int ti_job_run(ti_thing_t * thing, mp_unp_t * up, uint64_t ev_id);

#endif  /* TI_JOB_H_ */
