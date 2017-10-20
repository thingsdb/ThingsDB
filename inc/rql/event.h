/*
 * event.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_EVENT_H_
#define RQL_EVENT_H_

#define RQL_EVENT_STAT_UNINITIALIZED 0
#define RQL_EVENT_STAT_WAIT_ACCEPT 1
#define RQL_EVENT_STAT_ACCEPTED 2
#define RQL_EVENT_STAT_REJECTED 3
#define RQL_EVENT_STAT_DONE 4


typedef struct rql_event_s rql_event_t;

#include <inttypes.h>
#include <qpack.h>
#include <rql/events.h>
#include <rql/db.h>
#include <rql/raw.h>
#include <rql/prom.h>
#include <util/vec.h>
#include <util/imap.h>

rql_event_t * rql_event_create(rql_events_t * events);
void rql_event_destroy(rql_event_t * event);
void rql_event_new(rql_sock_t * sock, rql_pkg_t * pkg, ex_t * e);
void rql_event_init(rql_event_t * event);
void rql_event_raw(
        rql_event_t * event,
        const unsigned char * raw,
        size_t sz,
        ex_t * e);
int rql_event_run(rql_event_t * event);
int rql_event_done(rql_event_t * event);

struct rql_event_s
{
    uint64_t id;
    uint8_t flags;
    uint8_t status;
    rql_events_t * events;
    rql_db_t * target; // NULL for _rql or pointer to database
    rql_node_t * node;
    rql_sock_t * client;    // NULL or requesting client
    rql_raw_t * raw;
    imap_t * refelems;
    vec_t * tasks;  /* each task is a qp_res_t */
    qp_packer_t * result;
    rql_prom_t * prom;
};

#endif /* RQL_EVENT_H_ */
