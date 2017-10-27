/*
 * db.h
 *
 *  Created on: Sep 30, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_DB_H_
#define RQL_DB_H_

typedef struct rql_db_s  rql_db_t;

#include <stdint.h>
#include <rql/rql.h>
#include <rql/elem.h>
#include <util/imap.h>
#include <util/smap.h>
#include <util/guid.h>
#include <util/ex.h>

struct rql_db_s
{
    uint32_t ref;
    guid_t guid;
    rql_raw_t * name;
    rql_t * rql;
    imap_t * elems;
    smap_t * props;
    vec_t * access;
    rql_elem_t * root;
};

rql_db_t * rql_db_create(rql_t * rql, guid_t * guid, const rql_raw_t * name);
rql_db_t * rql_db_grab(rql_db_t * db);
void rql_db_drop(rql_db_t * db);
int rql_db_buid(rql_db_t * db);
int rql_db_name_check(const rql_raw_t * name, ex_t * e);
int rql_db_store(rql_db_t * db, const char * fn);
int rql_db_restore(rql_db_t * db, const char * fn);

#endif /* RQL_DB_H_ */
