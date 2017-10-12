/*
 * db.h
 *
 *  Created on: Sep 30, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_DB_H_
#define RQL_DB_H_

typedef struct rql_db_s  rql_db_t;

#include <inttypes.h>
#include <util/imap.h>
#include <util/smap.h>

struct rql_db_s
{
    uint64_t ref;
    char * name;
    imap_t * elems;
    smap_t * props;
};

rql_db_t * rql_db_create(void);
rql_db_t * rql_db_grab(rql_db_t * db);
void rql_db_drop(rql_db_t * db);

#endif /* RQL_DB_H_ */
