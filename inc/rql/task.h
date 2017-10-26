/*
 * task.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_TASK_H_
#define RQL_TASK_H_

#define RQL_TASK_N 15

/*
 * Max 60 tasks can be defined.
 */
typedef enum
{
    RQL_TASK_USER_CREATE,
    RQL_TASK_USER_DROP,
    RQL_TASK_USER_ALTER,
    RQL_TASK_DB_CREATE,
    RQL_TASK_DB_RENAME,
    RQL_TASK_DB_DROP,
    RQL_TASK_GRANT,
    RQL_TASK_REVOKE,
    RQL_TASK_SET_REDUNDANCY,
    RQL_TASK_NODE_ADD,
    RQL_TASK_NODE_REPLACE,
    RQL_TASK_SUBSCRIBE,
    RQL_TASK_UNSUBSCRIBE,
    RQL_TASK_PROPS_SET,
    RQL_TASK_PROPS_DEL,
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
#include <util/ex.h>

rql_task_t * rql_task_create(qp_res_t * res, ex_t * e);
void rql_task_destroy(rql_task_t * task);
rql_task_stat_e rql_task_run(
        rql_task_t * task,
        rql_event_t * event,
        rql_task_stat_e rc);
const char * rql_task_str(rql_task_t * task);

struct rql_task_s
{
    rql_task_e tp;
    qp_res_t * res;
};

#endif /* RQL_TASK_H_ */
