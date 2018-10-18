/*
 * proto.h
 */
#ifndef TI_PROTO_H_
#define TI_PROTO_H_

typedef enum
{
    TI_PROTO_ACK,       // empty
    TI_PROTO_RES,       // [{"_s": status, ...},...] or [status, status, status]
    TI_PROTO_ELEM,      // {"_i": <id>, ...}

    TI_PROTO_REJECT=64, // empty  (back-end only)

    TI_PROTO_AUTH_ERR,  // {error_msg: "..."} authentication or privileges
    TI_PROTO_NODE_ERR,  // {error_msg: "..."} node is unable to respond to the request
    TI_PROTO_TYPE_ERR,  // {error_msg: "..."} something is wrong with the request
    TI_PROTO_INDX_ERR,  // {error_msg: "..."} index error something cannot be found etc.
    TI_PROTO_RUNT_ERR,  // {error_msg: "..."} runtime, anything like memory, disk errors

} ti_proto_e;


#endif /* TI_PROTO_H_ */
