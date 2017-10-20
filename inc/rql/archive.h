/*
 * archive.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_ARCHIVE_H_
#define RQL_ARCHIVE_H_

typedef struct rql_archive_s  rql_archive_t;

#include <inttypes.h>
#include <rql/event.h>
#include <util/vec.h>

struct rql_archive_s
{
    uint64_t offset;
    vec_t * rawev;      /* rql_raw_t */
};

rql_archive_t * rql_archive_create(void);
void rql_archive_destroy(rql_archive_t * archive);
int rql_archive_event(rql_archive_t * archive, rql_event_t * event);

#endif /* RQL_ARCHIVE_H_ */
