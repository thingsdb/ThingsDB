/*
 * ti/proto.h
 */
#ifndef TI_PROTO_H_
#define TI_PROTO_H_

#define TI_PROTO_EV_ID 0xffff

typedef enum
{
    /*
     * protocol definition for client connections
     */

    /*
     * 0x0000xxxx  0..15 fire and forgets from node to client
     */
    TI_PROTO_CLIENT_NODE_STATUS =0,     /* str_status                       */
    TI_PROTO_CLIENT_WATCH_INI   =1,     /* {event:x, thing: {#:x, ...}      */
    TI_PROTO_CLIENT_WATCH_UPD   =2,     /* {event:x. #:x, jobs:[] etc }     */
    TI_PROTO_CLIENT_WATCH_DEL   =3,     /* {#:x}                            */
    TI_PROTO_CLIENT_WARN        =4,     /* {warn_msg:..., warn_code: x}     */

    /*
     * 0x0001xxxx  16..31 client responses
     */
    TI_PROTO_CLIENT_RES_PING    =16,    /* empty */
    TI_PROTO_CLIENT_RES_AUTH    =17,    /* empty */
    TI_PROTO_CLIENT_RES_QUERY   =18,    /* [{}, {}, ...] */
    TI_PROTO_CLIENT_RES_WATCH   =19,    /* empty */
    TI_PROTO_CLIENT_RES_UNWATCH =20,    /* empty */
    TI_PROTO_CLIENT_RES_ERROR   =21,    /* {error_msg:..., error_code: x} */

    /*
     * 0x0010xxxx  32..63 client requests
     */
    TI_PROTO_CLIENT_REQ_PING    =32,    /* empty                            */
    TI_PROTO_CLIENT_REQ_AUTH    =33,    /* [user, pass] or token            */
    TI_PROTO_CLIENT_REQ_QUERY   =34,    /* [scope, query, {variable}]       */
    TI_PROTO_CLIENT_REQ_WATCH   =35,    /* [scope, thing id's....]}         */
    TI_PROTO_CLIENT_REQ_UNWATCH =36,    /* [scope, thing id's....]}         */
    TI_PROTO_CLIENT_REQ_RUN     =37,    /* [scope, procedure, arguments...] */


    /*
     * protocol definition for node connections
     */

    /*
     * 128..159 node fire and forgets
     */
    TI_PROTO_NODE_EVENT         =158,   /* event */
    TI_PROTO_NODE_EVENT_CANCEL  =159,   /* event_id */
    TI_PROTO_NODE_INFO          =160,   /* [...] */

    /*
     * 160..191 node requests
     */

    /* expects a client response which will be forwarded back to the client */
    TI_PROTO_NODE_REQ_QUERY     =162,   /* [user_id, [original]] */
    TI_PROTO_NODE_REQ_RUN       =163,   /* [user_id, [original]] */

    TI_PROTO_NODE_REQ_CONNECT   =176,   /* [...] */
    TI_PROTO_NODE_REQ_EVENT_ID  =177,   /* event id */
    TI_PROTO_NODE_REQ_AWAY      =178,   /* empty */
    TI_PROTO_NODE_REQ_SETUP     =180,   /* empty */
    TI_PROTO_NODE_REQ_SYNC      =181,   /* event_id */

    /* [scope_id, file_id, offset, bytes, more]
     * more is a boolean which is set to true in case the file is not yet
     * complete.
     */
    TI_PROTO_NODE_REQ_SYNCFPART =182,   /* full sync part */
    TI_PROTO_NODE_REQ_SYNCFDONE =183,   /* full sync completed */
    TI_PROTO_NODE_REQ_SYNCAPART =184,   /* archive sync part */
    TI_PROTO_NODE_REQ_SYNCADONE =185,   /* archive sync completed */
    TI_PROTO_NODE_REQ_SYNCEPART =186,   /* event sync part */
    TI_PROTO_NODE_REQ_SYNCEDONE =187,   /* event sync completed */

    /*
     * 192..223 node responses
     */
    TI_PROTO_NODE_RES_CONNECT   =208,   /* [node_id, status] */
    TI_PROTO_NODE_RES_EVENT_ID  =209,   /* empty, event id accepted */
    TI_PROTO_NODE_RES_AWAY      =210,   /* empty, away id accepted */
    TI_PROTO_NODE_RES_SETUP     =212,   /* ti_data */
    TI_PROTO_NODE_RES_SYNC      =213,   /* empty */
    TI_PROTO_NODE_RES_SYNCFPART =214,   /* [scope, file, offset]
                                           here offset is 0 in case no more
                                           data for the file is required
                                         */
    TI_PROTO_NODE_RES_SYNCFDONE =215,   /* empty, ack */
    TI_PROTO_NODE_RES_SYNCAPART =216,   /* [first, last, offset]
                                           here
                                           here offset is 0 in case no more
                                           data for the file is required
                                         */
    TI_PROTO_NODE_RES_SYNCADONE =217,   /* empty, ack */
    TI_PROTO_NODE_RES_SYNCEPART =218,   /* event */
    TI_PROTO_NODE_RES_SYNCEDONE =219,   /* empty, ack */


    /*
     * 224..255 node errors
     */
    TI_PROTO_NODE_ERR_RES           =240,   /* message */
    TI_PROTO_NODE_ERR_EVENT_ID      =241,   /* empty */
    TI_PROTO_NODE_ERR_AWAY          =242,   /* empty */

} ti_proto_enum_t;

#define TI_PROTO_NODE_REQ_FWD_TIMEOUT 120
#define TI_PROTO_NODE_REQ_CONNECT_TIMEOUT 5
#define TI_PROTO_NODE_REQ_EVENT_ID_TIMEOUT 60
#define TI_PROTO_NODE_REQ_AWAY_ID_TIMEOUT 5
#define TI_PROTO_NODE_REQ_SETUP_TIMEOUT 10
#define TI_PROTO_NODE_REQ_SYNC_TIMEOUT 10
#define TI_PROTO_NODE_REQ_SYNCFPART_TIMEOUT 10
#define TI_PROTO_NODE_REQ_SYNCFDONE_TIMEOUT 300
#define TI_PROTO_NODE_REQ_SYNCAPART_TIMEOUT 10
#define TI_PROTO_NODE_REQ_SYNCADONE_TIMEOUT 300
#define TI_PROTO_NODE_REQ_SYNCEPART_TIMEOUT 10
#define TI_PROTO_NODE_REQ_SYNCEDONE_TIMEOUT 300

const char * ti_proto_str(ti_proto_enum_t tp);

#endif /* TI_PROTO_H_ */
