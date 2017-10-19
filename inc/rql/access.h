/*
 * access.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_ACCESS_H_
#define RQL_ACCESS_H_

#include <inttypes.h>
#include <rql/user.h>
#include <util/vec.h>

int rql_access_grant(vec_t ** access, rql_user_t * user, uint64_t mask);
void rql_access_revoke(vec_t * access, rql_user_t * user, uint64_t mask);
_Bool rql_access_check(const vec_t * access, rql_user_t * user, uint64_t mask);

int rql_access_store(const vec_t * access, const char * fn);
int rql_access_restore(vec_t ** access, const vec_t * users, const char * fn);

#endif /* RQL_ACCESS_H_ */
