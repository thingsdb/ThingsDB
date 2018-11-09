/*
 * ti/proto.c
 */
#include <ti/proto.h>
#include <util/logger.h>

const char * ti_proto_str(ti_proto_e tp)
{
    switch (tp)
    {
    case TI_PROTO_CLIENT_CHANGE:            return "CLIENT_CHANGE";

    case TI_PROTO_CLIENT_REQ_PING:          return "CLIENT_REQ_PING";
    case TI_PROTO_CLIENT_REQ_AUTH:          return "CLIENT_REQ_AUTH";
    case TI_PROTO_CLIENT_REQ_QUERY:         return "CLIENT_REQ_QUERY";

    case TI_PROTO_CLIENT_RES_PING:          return "CLIENT_RES_PING";
    case TI_PROTO_CLIENT_RES_AUTH:          return "CLIENT_RES_AUTH";
    case TI_PROTO_CLIENT_RES_QUERY:         return "CLIENT_RES_QUERY";

    case TI_PROTO_CLIENT_ERR_MAX_QUOTA:     return "CLIENT_ERR_MAX_QUOTA";
    case TI_PROTO_CLIENT_ERR_AUTH:          return "CLIENT_ERR_AUTH";
    case TI_PROTO_CLIENT_ERR_FORBIDDEN:     return "CLIENT_ERR_FORBIDDEN";
    case TI_PROTO_CLIENT_ERR_INDEX:         return "CLIENT_ERR_INDEX";
    case TI_PROTO_CLIENT_ERR_BAD_REQUEST:   return "CLIENT_ERR_BAD_REQUEST";
    case TI_PROTO_CLIENT_ERR_QUERY:         return "CLIENT_ERR_QUERY";
    case TI_PROTO_CLIENT_ERR_NODE:          return "CLIENT_ERR_NODE";
    case TI_PROTO_CLIENT_ERR_INTERNAL:      return "CLIENT_ERR_INTERNAL";

    case TI_PROTO_NODE_EVENT_CANCEL:        return "NODE_EVENT_CANCEL";
    case TI_PROTO_NODE_STATS:               return "NODE_STATS";

    case TI_PROTO_NODE_REQ_QUERY:           return "NODE_REQ_QUERY";

    case TI_PROTO_NODE_REQ_CONNECT:         return "NODE_REQ_CONNECT";
    case TI_PROTO_NODE_REQ_EVENT_ID:        return "NODE_REQ_EVENT_ID";

    case TI_PROTO_NODE_RES_QUERY:           return "NODE_RES_QUERY";

    case TI_PROTO_NODE_RES_CONNECT:         return "NODE_RES_CONNECT";
    case TI_PROTO_NODE_RES_EVENT_ID:        return "NODE_RES_EVENT_ID";

    case TI_PROTO_NODE_ERR_MAX_QUOTA:       return "NODE_ERR_MAX_QUOTA";
    case TI_PROTO_NODE_ERR_AUTH:            return "NODE_ERR_AUTH";
    case TI_PROTO_NODE_ERR_FORBIDDEN:       return "NODE_ERR_FORBIDDEN";
    case TI_PROTO_NODE_ERR_INDEX:           return "NODE_ERR_INDEX";
    case TI_PROTO_NODE_ERR_BAD_REQUEST:     return "NODE_ERR_BAD_REQUEST";
    case TI_PROTO_NODE_ERR_QUERY:           return "NODE_ERR_QUERY";
    case TI_PROTO_NODE_ERR_NODE:            return "NODE_ERR_NODE";
    case TI_PROTO_NODE_ERR_INTERNAL:        return "NODE_ERR_INTERNAL";

    case TI_PROTO_NODE_ERR_CONNECT:         return "NODE_ERR_CONNECT";
    case TI_PROTO_NODE_ERR_EVENT_ID:        return "NODE_ERR_EVENT_ID";

    }
    log_error("asking a string for an unexpected protocol type: `%d`", tp);
    return "UNKNOWN TYPE";
}
