/*
 * ti/proto.t.h
 */
#ifndef TI_PROTO_T_H_
#define TI_PROTO_T_H_

#define TI_PROTO_EV_ID 0xffff

typedef enum
{
    /*
     * protocol definition for client connections
     */

    /*
     * 0x0000xxxx  0..15 fire and forgets from node to client
     */
    TI_PROTO_CLIENT_NODE_STATUS =0,     /* {id:x status:...}                */
    /* 1..4 : Old watch protocol */
    TI_PROTO_CLIENT_WARN        =5,     /* {warn_msg:..., warn_code: x}     */
    TI_PROTO_CLIENT_ROOM_JOIN   =6,     /* {id:x data:...}                  */
    TI_PROTO_CLIENT_ROOM_LEAVE  =7,     /* {id:x data:...}                  */
    TI_PROTO_CLIENT_ROOM_EMIT   =8,     /* {id:x event:name args:[...] }    */
    TI_PROTO_CLIENT_ROOM_DELETE =9,     /* {id:x }                          */

    /*
     * 0x0001xxxx  16..31 client responses
     */
    TI_PROTO_CLIENT_RES_PONG    =16,    /* empty */
    TI_PROTO_CLIENT_RES_OK      =17,    /* empty */
    TI_PROTO_CLIENT_RES_DATA    =18,    /* ... */
    TI_PROTO_CLIENT_RES_ERROR   =19,    /* {error_msg:..., error_code: x}   */

    /*
     * 0x0010xxxx  32..63 client requests
     */
    TI_PROTO_CLIENT_REQ_PING    =32,    /* empty                            */
    TI_PROTO_CLIENT_REQ_AUTH    =33,    /* [user, pass] or token            */
    TI_PROTO_CLIENT_REQ_QUERY   =34,    /* [scope, query, {variable}]       */
    /* 35..36 : Old watch protocol */
    _TI_PROTO_CLIENT_DEP_35     =35,    /* TODO (COMPAT) */
    _TI_PROTO_CLIENT_DEP_36     =36,    /* TODO (COMPAT) */

    TI_PROTO_CLIENT_REQ_RUN     =37,    /* [scope, procedure, args...] */
    TI_PROTO_CLIENT_REQ_JOIN    =38,    /* [scope, room id's....]}          */
    TI_PROTO_CLIENT_REQ_LEAVE   =39,    /* [scope, room id's....]}          */
    TI_PROTO_CLIENT_REQ_EMIT    =40,    /* [scope, room_id, event, args...] */

    /*
     * 64..127 modules range
     */
    TI_PROTO_MODULE_CONF           =64,    /* data, initialize extension */
    TI_PROTO_MODULE_CONF_OK        =65,    /* empty */
    TI_PROTO_MODULE_CONF_ERR       =66,    /* empty */
    TI_PROTO_MODULE_REQ            =80,    /* data, request */
    TI_PROTO_MODULE_RES            =81,    /* data, response */
    TI_PROTO_MODULE_ERR            =82,    /* [err_nr, message] */

    /*
     * protocol definition for node connections
     */

    /*
     * 128..159 node fire and forgets
     */
    TI_PROTO_NODE_CHANGE            =128,   /* change */
    TI_PROTO_NODE_INFO              =129,   /* [...] */
    TI_PROTO_NODE_MISSING_CHANGE    =130,   /* change_id */
    /* 131..132 : Old watch protocol */
    TI_PROTO_NODE_FWD_TIMER         =133,   /* [scope_id, timer_id] */
    TI_PROTO_NODE_OK_TIMER          =134,   /* [scope_id, timer_id, next_ts] */
    TI_PROTO_NODE_EX_TIMER          =135,   /* [scope_id, timer_id, next_ts,
                                            err_code, err_msg] */
    TI_PROTO_NODE_ROOM_EMIT         =136,   /* {id:.., args: [..]} */
    /*
     * 160..191 node requests
     */

    /* expects a client response which will be forwarded back to the client */
    TI_PROTO_NODE_REQ_QUERY     =160,   /* [user_id, [original]] */
    TI_PROTO_NODE_REQ_RUN       =161,   /* [user_id, [original]] */

    TI_PROTO_NODE_REQ_CONNECT   =168,   /* [...] */
    TI_PROTO_NODE_REQ_CHANGE_ID =169,   /* change id */
    TI_PROTO_NODE_REQ_AWAY      =170,   /* empty */
    TI_PROTO_NODE_REQ_SETUP     =171,   /* empty */
    TI_PROTO_NODE_REQ_SYNC      =172,   /* change_id */

    /* [scope_id, file_id, offset, bytes, more]
     * more is a boolean which is set to true in case the file is not yet
     * complete.
     */
    TI_PROTO_NODE_REQ_SYNCFPART =173,   /* full sync part */
    TI_PROTO_NODE_REQ_SYNCFDONE =174,   /* full sync completed */
    TI_PROTO_NODE_REQ_SYNCAPART =175,   /* archive sync part */
    TI_PROTO_NODE_REQ_SYNCADONE =176,   /* archive sync completed */
    TI_PROTO_NODE_REQ_SYNCEPART =177,   /* changes sync part */
    TI_PROTO_NODE_REQ_SYNCEDONE =178,   /* changes sync completed */

    /*
     * 192..223 node responses
     */
    TI_PROTO_NODE_RES_CONNECT   =192,   /* [node_id, status] */
    TI_PROTO_NODE_RES_ACCEPT    =193,   /* quorum request accepted */
    TI_PROTO_NODE_RES_SETUP     =194,   /* ti_data */
    TI_PROTO_NODE_RES_SYNC      =195,   /* empty */
    TI_PROTO_NODE_RES_SYNCFPART =196,   /* [scope, file, offset]
                                           here offset is 0 in case no more
                                           data for the file is required
                                         */
    TI_PROTO_NODE_RES_SYNCFDONE =197,   /* empty, ack */
    TI_PROTO_NODE_RES_SYNCAPART =198,   /* [first, last, offset]
                                           here
                                           here offset is 0 in case no more
                                           data for the file is required
                                         */
    TI_PROTO_NODE_RES_SYNCADONE =199,   /* empty, ack */
    TI_PROTO_NODE_RES_SYNCEPART =200,   /* changes */
    TI_PROTO_NODE_RES_SYNCEDONE =201,   /* empty, ack */


    /*
     * 224..255 node errors
     */
    TI_PROTO_NODE_ERR               =224,   /* empty, error */
    TI_PROTO_NODE_ERR_RES           =225,   /* message */
    TI_PROTO_NODE_ERR_REJECT        =226,   /* quorum request rejected */
    TI_PROTO_NODE_ERR_COLLISION     =227,   /* `n`, quorum request collision */

} ti_proto_enum_t;

#define TI_PROTO_NODE_REQ_FWD_TIMEOUT 120
#define TI_PROTO_NODE_REQ_CONNECT_TIMEOUT 5
#define TI_PROTO_NODE_REQ_CHANGE_ID_TIMEOUT 60
#define TI_PROTO_NODE_REQ_AWAY_ID_TIMEOUT 5
#define TI_PROTO_NODE_REQ_SETUP_TIMEOUT 10
#define TI_PROTO_NODE_REQ_SYNC_TIMEOUT 10
#define TI_PROTO_NODE_REQ_SYNCFPART_TIMEOUT 10
#define TI_PROTO_NODE_REQ_SYNCFDONE_TIMEOUT 300
#define TI_PROTO_NODE_REQ_SYNCAPART_TIMEOUT 10
#define TI_PROTO_NODE_REQ_SYNCADONE_TIMEOUT 300
#define TI_PROTO_NODE_REQ_SYNCEPART_TIMEOUT 10
#define TI_PROTO_NODE_REQ_SYNCEDONE_TIMEOUT 300

#endif /* TI_PROTO_T_H_ */
