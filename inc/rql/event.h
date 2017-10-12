/*
 * event.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_EVENT_H_
#define RQL_EVENT_H_

#define RQL_EVENT_STATUS_INIT 0
#define RQL_EVENT_STATUS_WAIT_ACCEPT 1
#define RQL_EVENT_STATUS_ACCEPTED 2

typedef struct rql_event_s rql_event_t;

#include <inttypes.h>
#include <util/vec.h>
#include <util/imap.h>

rql_event_t * rql_create_event(rql_t * rql);
void rql_event_destroy(rql_event_t * event);
int rql_event_raw(
        rql_event_t * event,
        const unsigned char * raw,
        size_t sz,
        ex_t * e);


struct rql_event_s
{
    uint64_t id;
    uint8_t flags;
    uint8_t status;
    rql_t * rql;
    rql_db_t * target; // NULL for _rql or pointer to database
    rql_node_t * node;
    unsigned char * raw;
    size_t raw_sz;
    imap_t * refelems;
    vec_t * tasks;
};


#endif /* RQL_EVENT_H_ */
