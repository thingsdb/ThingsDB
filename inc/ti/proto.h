/*
 * proto.h
 */
#ifndef TI_PROTO_H_
#define TI_PROTO_H_

/*
 * TODO: make tests for checking if `node-responses-128` still map to client
 *       requests. this is required when we forward a query, and alter the
 *       response when returning data to the client.
 */

typedef enum
{
    /*
     * protocol definition for client connections
     */

    /*
     * 0..31 UNUSED
     */

    /*
     * 32..47 client requests mapping to node requests
     */

    TI_PROTO_CLIENT_REQ_PING    =32,    /* empty                     */
    TI_PROTO_CLIENT_REQ_AUTH    =33,    /* [username, password]      */
    TI_PROTO_CLIENT_REQ_QUERY   =34,    /* {target:.. query:.. blobs: []} */

    /*
     * 48..63 client only requests
     */

    /*
     * 64..79 client responses mapping to node responses
     */

    TI_PROTO_CLIENT_RES_PING    =64,    /* empty */
    TI_PROTO_CLIENT_RES_AUTH    =65,    /* empty */
    TI_PROTO_CLIENT_RES_QUERY   =66,    /* [{}, {}, ...] */

    /*
     * 80..95 client only responses
     */

    /*
     * 96..111 client errors mapping to node errors
     */

    /* authentication failed or request without authentication */
    TI_PROTO_CLIENT_ERR_AUTH        =96,
    /* no access for the requested task */
    TI_PROTO_CLIENT_ERR_FORBIDDEN   =97,
    /* query syntax error */
    TI_PROTO_CLIENT_ERR_INDEX       =98,
    /* invalid request, incorrect package type, invalid QPack data */
    TI_PROTO_CLIENT_ERR_BAD_REQUEST =99,
    /* node is (currently) unable to respond to the request */
    TI_PROTO_CLIENT_ERR_QUERY       =100,
    /* not found, maybe because due to no access */
    TI_PROTO_CLIENT_ERR_NODE        =101,
    /* internal server error, for example allocation error */
    TI_PROTO_CLIENT_ERR_INTERNAL    =102,

    /*
     * 112..127 client errors only
     */

    /*
     * protocol definition for node connections
     */

    /*
     * 128..159 UNUSED
     */

    /*
     * 160..175 node requests mapping to client requests
     */

    TI_PROTO_NODE_REQ_PING      =160,   /* empty */
    TI_PROTO_NODE_REQ_AUTH      =161,   /* [node_id]     */
    TI_PROTO_NODE_REQ_QUERY     =162,   /* [user_id, {query...}] */

    /*
     * 176..191 node only requests
     */

    /*
     * 192..207 node responses mapping to client responses
     */

    TI_PROTO_NODE_RES_PING      =192,   /* empty */
    TI_PROTO_NODE_RES_AUTH      =193,   /* empty */
    TI_PROTO_NODE_RES_QUERY     =194,   /* [{}, {}, ...] */

    /*
     * 208..223 node only responses
     */

    /*
     * 224..239 node errors mapping to client errors
     */

    /* authentication failed or request without authentication */
    TI_PROTO_NODE_ERR_AUTH          =224,
    /* no access for the requested task */
    TI_PROTO_NODE_ERR_FORBIDDEN     =225,
    /* query syntax error */
    TI_PROTO_NODE_ERR_INDEX         =226,
    /* invalid request, incorrect package type, invalid QPack data */
    TI_PROTO_NODE_ERR_BAD_REQUEST   =227,
    /* node is (currently) unable to respond to the request */
    TI_PROTO_NODE_ERR_QUERY         =228,
    /* not found, maybe because due to no access */
    TI_PROTO_NODE_ERR_NODE          =229,
    /* internal server error, for example allocation error */
    TI_PROTO_NODE_ERR_INTERNAL      =230,

    /*
     * 240..255 node only errors
     */

    TI_PROTO_NODE_ERR_REJECT        =240,

} ti_proto_e;

#define TI_PROTO_NODE_REQ_QUERY_TIMEOUT 120

const char * ti_proto_str(ti_proto_e tp);

#endif /* TI_PROTO_H_ */
