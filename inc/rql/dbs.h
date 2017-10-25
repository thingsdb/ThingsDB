/*
 * dbs.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_DBS_H_
#define RQL_DBS_H_

#include <rql/db.h>
#include <rql/pkg.h>
#include <rql/raw.h>
#include <rql/sock.h>
#include <util/ex.h>
#include <util/vec.h>

rql_db_t * rql_dbs_get_by_name(const vec_t * dbs, const rql_raw_t * name);
rql_db_t * rql_dbs_get_by_obj(const vec_t * dbs, const qp_obj_t * target);
void rql_dbs_get(rql_sock_t * sock, rql_pkg_t * pkg, ex_t * e);

//int rql_dbs_store(const vec_t * dbs, const char * fn);
//int rql_dbs_restore(vec_t ** dbs, const char * fn);

#endif /* RQL_DBS_H_ */
