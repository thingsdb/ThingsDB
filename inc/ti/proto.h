/*
 * proto.h
 */
#ifndef TI_PROTO_H_
#define TI_PROTO_H_

typedef enum
{
    /*
     * protocol definition for client connections
     */

    /* client requests */
    TI_PROTO_CLIENT_REQ_PING    =32,    /* empty                     */
    TI_PROTO_CLIENT_REQ_AUTH    =33,    /* [username, password]      */
    TI_PROTO_CLIENT_REQ_QUERY   =34,    /* {target:.. query:.. blobs: []} */

    /* client responses */
    TI_PROTO_CLIENT_RES_PING    =64,    /* empty */
    TI_PROTO_CLIENT_RES_AUTH    =65,    /* empty */
    TI_PROTO_CLIENT_RES_QUERY   =66,    /* [{}, {}, ...] */

    /*
     * client errors
     */
    /* authentication failed or request without authentication */
    TI_PROTO_CLIENT_ERR_AUTH    =96,
    /* node is unable to respond to the request */
    TI_PROTO_CLIENT_ERR_NODE    =97,
    /* query syntax error */
    TI_PROTO_CLIENT_ERR_QUERY   =98,
    /* target not found or no access */
    TI_PROTO_CLIENT_ERR_TARGET  =99,
    /* runtime error, for example allocation error */
    TI_PROTO_CLIENT_ERR_RUNTIME =100,


    /*
     * protocol definition for node connections
     */
    TI_PROTO_NODE_REQ_PING      =128,   /* empty */
    TI_PROTO_NODE_REQ_AUTH_REQ  =129,   /* {user:.. password:..}     */

} ti_proto_e;


const char * ti_proto_str(ti_proto_e tp);

#endif /* TI_PROTO_H_ */
