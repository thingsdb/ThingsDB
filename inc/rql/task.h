/*
 * task.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_TASK_H_
#define RQL_TASK_H_

typedef enum
{
    RQL_TASK_CREATE_USER
} rql_task_e;

typedef struct rql_task_s rql_task_t;

struct rql_task_s
{
    rql_task_e type;
};


#endif /* RQL_TASK_H_ */
