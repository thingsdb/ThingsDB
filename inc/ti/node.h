/*
 * ti/node.h
 */
#ifndef TI_NODE_H_
#define TI_NODE_H_

typedef enum
{
    TI_NODE_STAT_OFFLINE,
    TI_NODE_STAT_CONNECTING,
    TI_NODE_STAT_AWAY,
    TI_NODE_STAT_AWAY_SOON,  /* few seconds before going away,
                                back-end still accepts queries */
    TI_NODE_STAT_READY
} ti_node_status_t;

typedef struct ti_node_s ti_node_t;

#include <uv.h>
#include <stdint.h>
#include <ti/stream.h>
#include <ti/pkg.h>
#include <ti/lookup.h>
#include <util/imap.h>

struct ti_node_s
{
    uint32_t ref;
    uint32_t next_retry;            /* retry connect when >= to next retry */
    uint32_t retry_counter;         /* connection retry counter */
    uint8_t id;  /* node id, equal to the index in tin->nodes and each node
                    contains the same order since the lookup is based on this
                    id. When deciding which node wins over an equal event id
                    request, the higher ((node->id + event->id) % n-nodes)
                    is the winner. */
    uint8_t status;
    uint8_t maintn;
    ti_stream_t * stream;
    struct sockaddr_storage addr;
};

ti_node_t * ti_node_create(uint8_t id, struct sockaddr_storage * addr);
void ti_node_drop(ti_node_t * node);
const char * ti_node_name(ti_node_t * node);
int ti_node_connect(ti_node_t * node);
int ti_node_write_stats(ti_node_t * node);
int ti_node_write(ti_node_t * node, ti_pkg_t * pkg);
ti_node_t * ti_node_winner(ti_node_t * node_a, ti_node_t * node_b, uint64_t u);
static inline _Bool ti_node_manages_id(
        ti_node_t * node,
        ti_lookup_t * lookup,
        uint64_t id);

static inline _Bool ti_node_manages_id(
        ti_node_t * node,
        ti_lookup_t * lookup,
        uint64_t id)
{
    return lookup->mask_[id % lookup->n_] & (1 << node->id);
}



#endif /* TI_NODE_H_ */
