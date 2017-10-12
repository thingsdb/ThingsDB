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
    RQL_FRONT_REQ
} rql_front_req_e;

typedef enum
{
    RQL_FRONT_ACK,      // empty
    RQL_FRONT_ERR=64,   // empty
    RQL_FRONT_REJECT,   // empty
    RQL_FRONT_AUTH_ERR, // {error_msg: "..."} authentication or privileges
    RQL_FRONT_NODE_ERR, // {error_msg: "..."} node is unable to respond to the request
    RQL_FRONT_TYPE_ERR, // {error_msg: "..."} something is wrong with the request
    RQL_FRONT_INDX_ERR, // {error_msg: "..."} index error something cannot be found etc.
    RQL_FRONT_RUNT_ERR, // {error_msg: "..."} runtime, anything like memory, disk errors

} rql_front_res_e;

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

#endif /* RQL_FRONT_H_ */


