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
#include <util/ex.h>

extern const char * rql_user_def_name;
extern const char * rql_user_def_pass;
extern const unsigned int rql_min_name;  // length including terminator
extern const unsigned int rql_max_name;  // length including terminator
extern const unsigned int rql_min_pass;  // length including terminator
extern const unsigned int rql_max_pass;  // length including terminator

rql_user_t * rql_user_create(const char * name, const char * encrpass);
rql_user_t * rql_user_grab(rql_user_t * user);
void rql_user_drop(rql_user_t * user);
int rql_user_name_check(const char * name, ex_t * e);
int rql_user_pass_check(const char * pass, ex_t * e);
int rql_user_rename(rql_user_t * user, const char * name);
int rql_user_set_pass(rql_user_t * user, const char * pass);

struct rql_user_s
{
    uint64_t ref;
    char * name;
    char * pass;
};

#endif /* RQL_USER_H_ */
