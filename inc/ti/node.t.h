/*
 * ti/node.t.h
 */
#ifndef TI_NODE_T_H_
#define TI_NODE_T_H_

typedef enum
{
    /*
     * Off-line: node is not connected.
     */
    TI_NODE_STAT_OFFLINE        =0,

    /*
     * Connecting: changed to connecting when trying to setup a connection
     *             to a node. This-node can never have status connection.
     */
    TI_NODE_STAT_CONNECTING     =1<<0,

    /*
     * Connecting: changed to connecting when trying to setup a connection
     *             to a node. This-node can never have status connection.
     */
    TI_NODE_STAT_CONNECTED      =1<<1,

    /*
     * Building: this state has a node when it starts building ThingsDB. It is
     *           a lower state than synchronizing since here we might not even
     *           know which nodes are available. (and This node might not even
     *           exist)
     */
    TI_NODE_STAT_BUILDING       =1<<2,

    /*
     * Shutting-Down: Few seconds before going off-line, the node still accepts
     *                requests on existing connections but all nodes and
     *                clients should change to another connection.
     */
    TI_NODE_STAT_SHUTTING_DOWN  =1<<3,

    /*
     * Synchronizing: We have at least the known nodes. In this mode we can
     *                accept (or reject) change id's, although processing
     *                changes and queries is not possible.
     */
    TI_NODE_STAT_SYNCHRONIZING  =1<<4,

    /*
     * Away: In this mode we cannot access collection data. Client queries are
     *       forwarded and changes etc. are stored to disk. We can still accept
     *       or reject change id's and append new changes to the queue.
     */
    TI_NODE_STAT_AWAY           =1<<5,

    /*
     * Away-Soon: Few seconds before going into `away` mode. The node still
     *            accepts requests from clients but they will be forwarded to
     *            another node. Back-end request are still handled but nodes
     *            should stop asking for collection data (attributes).
     */
    TI_NODE_STAT_AWAY_SOON      =1<<6,

    /*
     * Ready: This node is ready to accept and handle requests.
     */
    TI_NODE_STAT_READY          =1<<7,
} ti_node_status_t;

/*
 * Size TI_NODE_INFO_PK_SZ for node status info.
 *  {
 *      next_thing_id,
 *      ccid,
 *      scid,
 *      status
 *      zone,
 *      port,
 *      version
 *  }
 */
#define TI_NODE_INFO_PK_SZ 128

typedef struct ti_node_s ti_node_t;

#include <stdint.h>
#include <ti/stream.t.h>
#include <util/cryptx.h>

struct ti_node_s
{
    uint32_t ref;
    uint32_t id;

    uint16_t port;
    uint16_t pad0_;
    uint16_t syntax_ver;            /* syntax version */
    uint8_t status;
    uint8_t zone;                   /* zone info */

    uint32_t next_retry;            /* retry connect when >= to next retry */
    uint32_t retry_counter;         /* connection retry counter */

    uint64_t ccid;                 /* last committed change id */
    uint64_t scid;                 /* last stored change id on disk */
    uint64_t next_free_id;
    ti_stream_t * stream;           /* borrowed reference */

    /*
     * TODO: add warning flags, like:
     *      - low on memory
     *      - low on disk space
     *      - unreachable (set from another node)
     *    uint16_t flags;
     */
    char * addr;                    /* host-name or IP address */
    char secret[CRYPTX_SZ];         /* null terminated encrypted secret */
};

#endif /* TI_NODE_T_H_ */
