/*
 * front.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_FRONT_H_
#define RQL_FRONT_H_

typedef enum
{
    RQL_FRONT_PING,     // empty
    RQL_FRONT_AUTH,     // [user, password]
    RQL_FRONT_EVENT,    // {"target": [tasks...]}
} rql_front_req_e;



typedef struct rql_front_s  rql_front_t;

#include <rql/rql.h>
#include <rql/pkg.h>

struct rql_front_s
{
    rql_sock_t * sock;
};

rql_front_t * rql_front_create(rql_t * rql);
void rql_front_destroy(rql_front_t * front);
int rql_front_listen(rql_front_t * front);
int rql_front_write(rql_sock_t * sock, rql_pkg_t * pkg);

const char * rql_front_req_str(rql_front_req_e);

#endif /* RQL_FRONT_H_ */


