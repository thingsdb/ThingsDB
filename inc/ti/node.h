/*
 * ti/node.h
 */
#ifndef TI_NODE_H_
#define TI_NODE_H_

#include <stdint.h>
#include <ti/node.t.h>
#include <ti/val.t.h>
#include <ti/version.h>
#include <util/mpack.h>

ti_node_t * ti_node_create(
        uint32_t id,
        uint8_t zone,
        uint16_t port,
        const char * addr,
        const char * secret);
void ti_node_drop(ti_node_t * node);
void ti_node_upd_node(ti_node_t * node, uint16_t port, mp_obj_t * node_name);
const char * ti_node_name(ti_node_t * node);
const char * ti_node_status_str(ti_node_status_t status);
void ti_node_connect(ti_node_t * node);
int ti_node_info_to_pk(ti_node_t * node, msgpack_packer * pk);
ti_val_t * ti_node_as_mpval(ti_node_t * node);
int ti_node_status_from_unp(ti_node_t * node, mp_unp_t * up);

static inline int ti_node_status_to_pk(ti_node_t * node, msgpack_packer * pk)
{
    return -(
        msgpack_pack_array(pk, 7) ||
        msgpack_pack_uint64(pk, node->next_free_id) ||
        msgpack_pack_uint64(pk, node->cevid) ||
        msgpack_pack_uint64(pk, node->sevid) ||
        msgpack_pack_uint8(pk, node->status) ||
        msgpack_pack_uint8(pk, node->zone) ||
        msgpack_pack_uint16(pk, node->port) ||
        msgpack_pack_uint8(pk, TI_VERSION_SYNTAX)
    );
}

#endif /* TI_NODE_H_ */
