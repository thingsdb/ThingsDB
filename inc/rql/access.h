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
int rql_access_check(vec_t * access, rql_user_t * user, uint64_t mask);

typedef struct rql_access_s rql_access_t;

struct rql_access_s
{
    vec_t * auth;
};

#endif /* RQL_ACCESS_H_ */
