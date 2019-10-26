/*
 * ti/proto.c
 */
#include <ti/proto.h>
#include <util/logger.h>

const char * ti_proto_str(ti_proto_enum_t tp)
{
    switch (tp)
    {
    case TI_PROTO_CLIENT_NODE_STATUS:       return "CLIENT_NODE_STATUS";
    case TI_PROTO_CLIENT_WATCH_INI:         return "CLIENT_WATCH_INI";
    case TI_PROTO_CLIENT_WATCH_UPD:         return "CLIENT_WATCH_UPD";
    case TI_PROTO_CLIENT_WATCH_DEL:         return "CLIENT_WATCH_DEL";
    case TI_PROTO_CLIENT_WARN:              return "CLIENT_WARN";

    case TI_PROTO_CLIENT_RES_PING:          return "CLIENT_RES_PING";
    case TI_PROTO_CLIENT_RES_AUTH:          return "CLIENT_RES_AUTH";
    case TI_PROTO_CLIENT_RES_QUERY:         return "CLIENT_RES_QUERY";
    case TI_PROTO_CLIENT_RES_WATCH:         return "CLIENT_RES_WATCH";
    case TI_PROTO_CLIENT_RES_UNWATCH:       return "CLIENT_RES_UNWATCH";
    case TI_PROTO_CLIENT_RES_ERROR:         return "CLIENT_RES_ERROR";

    case TI_PROTO_CLIENT_REQ_PING:          return "CLIENT_REQ_PING";
    case TI_PROTO_CLIENT_REQ_AUTH:          return "CLIENT_REQ_AUTH";
    case TI_PROTO_CLIENT_REQ_QUERY:         return "CLIENT_REQ_QUERY";
    case TI_PROTO_CLIENT_REQ_WATCH:         return "CLIENT_REQ_WATCH";
    case TI_PROTO_CLIENT_REQ_UNWATCH:       return "CLIENT_REQ_UNWATCH";
    case TI_PROTO_CLIENT_REQ_RUN:           return "CLIENT_REQ_RUN";

    case TI_PROTO_NODE_EVENT:               return "NODE_EVENT";
    case TI_PROTO_NODE_INFO:                return "NODE_INFO";

    case TI_PROTO_NODE_REQ_QUERY:           return "NODE_REQ_QUERY";
    case TI_PROTO_NODE_REQ_RUN:             return "NODE_REQ_RUN";

    case TI_PROTO_NODE_REQ_CONNECT:         return "NODE_REQ_CONNECT";
    case TI_PROTO_NODE_REQ_EVENT_ID:        return "NODE_REQ_EVENT_ID";
    case TI_PROTO_NODE_REQ_AWAY:            return "NODE_REQ_AWAY";
    case TI_PROTO_NODE_REQ_SETUP:           return "NODE_REQ_SETUP";
    case TI_PROTO_NODE_REQ_SYNC:            return "NODE_REQ_SYNC";
    case TI_PROTO_NODE_REQ_SYNCFPART:       return "NODE_REQ_SYNCFPART";
    case TI_PROTO_NODE_REQ_SYNCFDONE:       return "NODE_REQ_SYNCFDONE";
    case TI_PROTO_NODE_REQ_SYNCAPART:       return "NODE_REQ_SYNCAPART";
    case TI_PROTO_NODE_REQ_SYNCADONE:       return "NODE_REQ_SYNCADONE";
    case TI_PROTO_NODE_REQ_SYNCEPART:       return "NODE_REQ_SYNCEPART";
    case TI_PROTO_NODE_REQ_SYNCEDONE:       return "NODE_REQ_SYNCEDONE";

    case TI_PROTO_NODE_RES_CONNECT:         return "NODE_RES_CONNECT";
    case TI_PROTO_NODE_RES_ACCEPT:          return "NODE_RES_ACCEPT";
    case TI_PROTO_NODE_RES_SETUP:           return "NODE_RES_SETUP";
    case TI_PROTO_NODE_RES_SYNC:            return "NODE_RES_SYNC";
    case TI_PROTO_NODE_RES_SYNCFPART:       return "NODE_RES_SYNCFPART";
    case TI_PROTO_NODE_RES_SYNCFDONE:       return "NODE_RES_SYNCFDONE";
    case TI_PROTO_NODE_RES_SYNCAPART:       return "NODE_RES_SYNCAPART";
    case TI_PROTO_NODE_RES_SYNCADONE:       return "NODE_RES_SYNCADONE";
    case TI_PROTO_NODE_RES_SYNCEPART:       return "NODE_RES_SYNCEPART";
    case TI_PROTO_NODE_RES_SYNCEDONE:       return "NODE_RES_SYNCEDONE";

    case TI_PROTO_NODE_ERR_RES:             return "NODE_ERR_RES";
    case TI_PROTO_NODE_ERR_REJECT:          return "NODE_ERR_REJECT";
    case TI_PROTO_NODE_ERR_COLLISION:       return "NODE_ERR_COLLISION";

    }
    log_error("asking a string for an unexpected protocol type: `%d`", tp);
    return "UNKNOWN TYPE";
}
