/*
 * node.h
 */
#ifndef TI_NODE_H_
#define TI_NODE_H_

typedef enum
{
    TI_NODE_STAT_OFFLINE,
    TI_NODE_STAT_CONNECTED,
    TI_NODE_STAT_MAINT,
    TI_NODE_STAT_READY
} ti_node_status_t;

typedef struct ti_node_s ti_node_t;

#include <stdint.h>
#include <ti/stream.h>
#include <ti/pkg.h>
#include <ti/lookup.h>
#include <util/imap.h>

struct ti_node_s
{
    uint32_t ref;
    uint8_t id;  /* node id, equal to the index in tin->nodes and each node
                    contains the same order since the lookup is based on this
                    id. */
    uint8_t flags;
    uint8_t status;
    uint8_t maintn;
    uint32_t req_next_id;  /* TODO, what is thins?? was uint16_t*/
    uint32_t pad0;
    imap_t * reqs;
    ti_stream_t * stream;
    struct sockaddr_storage addr;
};

ti_node_t * ti_node_create(uint8_t id, struct sockaddr_storage * addr);
ti_node_t * ti_node_grab(ti_node_t * node);
void ti_node_drop(ti_node_t * node);
int ti_node_write(ti_node_t * node, ti_pkg_t * pkg);
static inline _Bool ti_node_has_id(
        ti_node_t * node,
        ti_lookup_t * lookup,
        uint64_t id);

static inline _Bool ti_node_has_id(
        ti_node_t * node,
        ti_lookup_t * lookup,
        uint64_t id)
{
    return lookup->mask_[id % lookup->n_] & (1 << node->id);
}

#endif /* TI_NODE_H_ */
