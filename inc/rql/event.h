/*
 * event.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_EVENT_H_
#define RQL_EVENT_H_

typedef struct rql_event_s rql_event_t;

#include <inttypes.h>
#include <util/vec.h>

struct rql_event_s
{
    uint64_t id;
    vec_t * tasks;
};


#endif /* RQL_EVENT_H_ */
