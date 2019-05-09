/*
 * ti/node.h
 */
#ifndef TI_NODE_H_
#define TI_NODE_H_

/* TODO: check through code if everything responds correctly to the status
 *       codes.
 */
typedef enum
{
    /*
     * Offline: node is not connected.
     */
    TI_NODE_STAT_OFFLINE,

    /*
     * Connecting: changed to connecting when trying to setup a connection
     *             to a node. This-node can never have status connection.
     */
    TI_NODE_STAT_CONNECTING,

    /*
     * Building: this state has a node when it starts building ThingsDB. It is
     *           a lower state than synchronizing since here we might not even
     *           know which nodes are available. (and This node might not even
     *           exist)
     */
    TI_NODE_STAT_BUILDING,

    /*
     * Synchronizing: We have at least the known nodes. In this mode we can
     *                accept (or reject) event id's, although processing events
     *                and queries is not possible.
     */
    TI_NODE_STAT_SYNCHRONIZING,

    /*
     * Away: In this mode we cannot access collection data. Client queries are
     *       forwarded and changes etc. are stored to disk. We can still accept
     *       or reject event id's and append new events to the queue.
     */
    TI_NODE_STAT_AWAY,

    /*
     * Away-Soon: Few seconds before going into `away` mode. The node still
     *            accepts requests from clients but they will be forwarded to
     *            another node. Back-end request are still handled but nodes
     *            should stop asking for collection data (attributes).
     */
    TI_NODE_STAT_AWAY_SOON,     /* few seconds before going away,
                                   back-end still accepts queries */
    /*
     * Shutting-Down: Few seconds before going offline, the node still accepts
     *                requests but all nodes and clients should change to
     *                another connection. Client request should be forwarded
     *                so no new event will be created by this node.
     */
    TI_NODE_STAT_SHUTTING_DOWN,

    /*
     * Ready: This node is ready to accept and handle requests.
     */
    TI_NODE_STAT_READY
} ti_node_status_t;


typedef struct ti_node_s ti_node_t;

#include <uv.h>
#include <stdint.h>
#include <ti/stream.h>
#include <ti/pkg.h>
#include <ti/rpkg.h>
#include <ti/lookup.h>
#include <util/imap.h>
#include <util/cryptx.h>

struct ti_node_s
{
    uint32_t ref;
    uint8_t id;  /* node id, equal to the index in tin->nodes and each node
                    contains the same order since the lookup is based on this
                    id. When deciding which node wins over an equal event id
                    request, the higher ((node->id + event->id) % n-nodes)
                    is the winner. */
    uint8_t status;
    uint8_t zone;                   /* zone info */
    uint8_t _pad0;
    uint32_t next_retry;            /* retry connect when >= to next retry */
    uint32_t retry_counter;         /* connection retry counter */
    uint64_t cevid;                 /* last committed event id */
    uint64_t sevid;                 /* last stored event id */
    uint64_t next_thing_id;
    ti_stream_t * stream;           /* borrowed reference */
    uint16_t port;
    struct sockaddr_storage * sockaddr_;
    char addr[INET6_ADDRSTRLEN];    /* null terminated (last known) address */
    char secret[CRYPTX_SZ];         /* null terminated encrypted secret */
};

ti_node_t * ti_node_create(
        uint8_t id,
        uint8_t zone,
        uint16_t port,
        const char * addr,
        const char * secret);
void ti_node_drop(ti_node_t * node);
int ti_node_upd_addr_from_stream(
        ti_node_t * node,
        ti_stream_t * stream,
        uint16_t port);
const char * ti_node_name(ti_node_t * node);
const char * ti_node_status_str(ti_node_status_t status);
int ti_node_connect(ti_node_t * node);
ti_node_t * ti_node_winner(ti_node_t * node_a, ti_node_t * node_b, uint64_t u);
int ti_node_info_to_packer(ti_node_t * node, qp_packer_t ** packer);
int ti_node_info_from_unp(ti_node_t * node, qp_unpacker_t * unp);

#endif /* TI_NODE_H_ */
