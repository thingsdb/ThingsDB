/*
 * proto.c
 */
#include <ti/proto.h>
#include <util/logger.h>

const char * ti_proto_str(ti_proto_e tp)
{
    switch (tp)
    {
    case TI_PROTO_CLIENT_REQ_PING:      return "CLIENT_REQ_PING";
    case TI_PROTO_CLIENT_REQ_AUTH:      return "CLIENT_REQ_AUTH";
    case TI_PROTO_CLIENT_REQ_QUERY:     return "CLIENT_REQ_QUERY";
    case TI_PROTO_CLIENT_RES_PING:      return "CLIENT_RES_PING";
    case TI_PROTO_CLIENT_RES_AUTH:      return "CLIENT_RES_AUTH";
    case TI_PROTO_CLIENT_RES_QUERY:     return "CLIENT_RES_QUERY";
    case TI_PROTO_CLIENT_ERR_AUTH:      return "CLIENT_ERR_AUTH";
    case TI_PROTO_CLIENT_ERR_NODE:      return "CLIENT_ERR_NODE";
    case TI_PROTO_CLIENT_ERR_QUERY:     return "CLIENT_ERR_QUERY";
    case TI_PROTO_CLIENT_ERR_TARGET:    return "CLIENT_ERR_TARGET";
    case TI_PROTO_CLIENT_ERR_RUNTIME:   return "CLIENT_ERR_RUNTIME";
    case TI_PROTO_NODE_REQ_PING:        return "NODE_REQ_PING";
    case TI_PROTO_NODE_REQ_AUTH:        return "NODE_REQ_AUTH";
    case TI_PROTO_NODE_ERR_REJECT:      return "NODE_ERR_REJECT";
    }
    log_error("asking a string for an unexpected protocol type: `%d`", tp);
    return "UNKNOWN TYPE";
}
