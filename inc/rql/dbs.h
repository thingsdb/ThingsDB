/*
 * dbs.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_DBS_H_
#define RQL_DBS_H_

#include <rql/db.h>
#include <util/vec.h>

rql_db_t * rql_dbs_get_by_name(const vec_t * dbs, const char * name);

//int rql_dbs_store(const vec_t * dbs, const char * fn);
//int rql_dbs_restore(vec_t ** dbs, const char * fn);

#endif /* RQL_DBS_H_ */
