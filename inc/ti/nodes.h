/*
 * nodes.h
 */
#ifndef TI_NODES_H_
#define TI_NODES_H_

#include <uv.h>
#include <ti/node.h>
#include <ti/rpkg.h>
#include <util/vec.h>
#include <qpack.h>

typedef struct ti_nodes_s ti_nodes_t;

int ti_nodes_create(void);
void ti_nodes_destroy(void);
int ti_nodes_listen(void);
uint8_t ti_nodes_quorum(void);
_Bool ti_nodes_has_quorum(void);
void ti_nodes_write_rpkg(ti_rpkg_t * rpkg);
int ti_nodes_to_packer(qp_packer_t ** packer);
int ti_nodes_from_qpres(qp_res_t * qpnodes);
uint64_t ti_nodes_cevid(void);
uint64_t ti_nodes_sevid(void);
ti_node_t * ti_nodes_create_node(struct sockaddr_storage * addr);
ti_node_t * ti_nodes_node_by_id(uint8_t node_id);
ti_node_t * ti_nodes_get_away(void);
ti_node_t * ti_nodes_get_away_or_soon(void);
ti_node_t * ti_nodes_random_ready_node(void);
ti_node_t * ti_nodes_random_ready_node_for_id(uint64_t id);
void ti_nodes_pkg_cb(ti_stream_t * stream, ti_pkg_t * pkg);
int ti_nodes_info_to_packer(qp_packer_t ** packer);
ti_val_t * ti_nodes_info_as_qpval(void);

struct ti_nodes_s
{
    vec_t * vec;
    uv_tcp_t tcp;
    uint64_t cevid;         /* last committed event id by ALL nodes,
                               ti_archive_t saves this value to disk at
                               cleanup and is therefore responsible to set
                               the initial value at startup */
    uint64_t sevid;         /* last stored event id by ALL nodes,
                               ti_archive_t saves this value to disk at
                               cleanup and is therefore responsible to set
                               the initial value at startup */
    struct sockaddr_storage addr;
};

#endif /* TI_NODES_H_ */
