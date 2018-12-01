/*
 * ti/job.h
 */
#ifndef TI_JOB_H_
#define TI_JOB_H_

#include <qpack.h>
#include <ti/db.h>
#include <ti/thing.h>

int ti_job_run(ti_db_t * db, ti_thing_t * thing, qp_unpacker_t * unp);

#endif  /* TI_JOB_H_ */
