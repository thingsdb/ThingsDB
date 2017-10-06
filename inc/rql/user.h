/*
 * user.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_USER_H_
#define RQL_USER_H_

#define RQL_CFG_PATH_MAX 4096
#define RQL_CFG_ADDR_MAX 256

typedef struct rql_user_s  rql_user_t;

#include <inttypes.h>

const char * rql_user_def_name = "root";

rql_user_t * rql_user_create(const char * name);
rql_user_t * rql_user_grab(rql_user_t * user);
void rql_user_drop(rql_user_t * user);
int rql_user_rename(rql_user_t * user, const char * name);

struct rql_user_s
{
    uint64_t ref;
    char * name;
};

#endif /* RQL_USER_H_ */
