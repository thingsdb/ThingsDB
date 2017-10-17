/*
 * task.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_TASK_H_
#define RQL_TASK_H_

#define RQL_TASK_N 1

typedef enum
{
    RQL_TASK_CREATE_USER
} rql_task_e;

typedef enum
{
    RQL_TASK_ERR=-1,
    RQL_TASK_SUCCESS,
    RQL_TASK_FAILED,
    RQL_TASK_SKIPPED
} rql_task_stat_e;

typedef struct rql_task_s rql_task_t;

#include <qpack.h>
#include <rql/event.h>

rql_task_stat_e rql_task(
        qp_res_t * task,
        rql_event_t * event,
        rql_task_stat_e rc);

struct rql_task_s
{
    rql_task_e tp;
};


#endif /* RQL_TASK_H_ */
