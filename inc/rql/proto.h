/*
 * proto.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_PROTO_H_
#define RQL_PROTO_H_

typedef enum
{
    RQL_PROTO_ACK,      // empty

    RQL_PROTO_ERR=64,   // empty
    RQL_PROTO_REJECT,   // empty

    RQL_PROTO_AUTH_ERR, // {error_msg: "..."} authentication or privileges
    RQL_PROTO_NODE_ERR, // {error_msg: "..."} node is unable to respond to the request
    RQL_PROTO_TYPE_ERR, // {error_msg: "..."} something is wrong with the request
    RQL_PROTO_INDX_ERR, // {error_msg: "..."} index error something cannot be found etc.
    RQL_PROTO_RUNT_ERR, // {error_msg: "..."} runtime, anything like memory, disk errors

} rql_proto_e;


#endif /* RQL_PROTO_H_ */
