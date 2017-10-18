/*
 * users.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_USERS_H_
#define RQL_USERS_H_

#include <qpack.h>
#include <util/vec.h>
#include <util/ex.h>

rql_user_t * rql_users_auth(
        vec_t * users,
        qp_obj_t * name,
        qp_obj_t * pass,
        ex_t * e);
int rql_users_store(const vec_t * users, const char * fn);
int rql_users_restore(vec_t ** users, const char * fn);

#endif /* RQL_USERS_H_ */
