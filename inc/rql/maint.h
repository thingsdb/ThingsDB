/*
 * maint.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_MAINT_H_
#define RQL_MAINT_H_

#define RQL_MAINT_STAT_READY 0
#define RQL_MAINT_STAT_REG 1
#define RQL_MAINT_STAT_BUSY 2

typedef struct rql_maint_s  rql_maint_t;

#include <uv.h>
#include <inttypes.h>
#include <rql/rql.h>

rql_maint_t * rql_maint_create(rql_t * rql);
void rql_maint_destroy(rql_maint_t * maint);
int rql_maint_start(rql_maint_t * maint);
void rql_maint_stop(rql_maint_t * maint);

struct rql_maint_s
{
    rql_t * rql;
    uv_work_t work;
    uv_timer_t timer;
    uint8_t status;
    uint64_t last_commit;
};

#endif /* RQL_MAINT_H_ */


