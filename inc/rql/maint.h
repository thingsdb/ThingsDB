/*
 * maint.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_MAINT_H_
#define RQL_MAINT_H_

#define RQL_MAINT_STAT_READY 0
#define RQL_MAINT_STAT_BUSY 1

typedef struct rql_maint_s  rql_maint_t;

#include <uv.h>

struct rql_maint_s
{
    rql_t * rql;
    uv_work_t work;
    uv_timer_t timer;
    uint8_t status;
};

rql_maint_t * rql_maint_create(rql_t * rql);
void rql_maint_destroy(rql_maint_t * maint);

rql_maint_t * rql_maint_start(rql_maint_t * maint);

#endif /* RQL_MAINT_H_ */


