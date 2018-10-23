/*
 * nodes.h
 */
#ifndef TI_NODES_H_
#define TI_NODES_H_

#include <uv.h>
#include <ti/node.h>
#include <util/vec.h>
#include <qpack.h>

typedef struct ti_nodes_s ti_nodes_t;

int ti_nodes_create(void);
void ti_nodes_destroy(void);
int ti_nodes_listen(void);
_Bool ti_nodes_has_quorum(void);
int ti_nodes_to_packer(qp_packer_t ** packer);
int ti_nodes_from_qpres(qp_res_t * qpnodes);
ti_node_t * ti_nodes_create_node(struct sockaddr_storage * addr);
ti_node_t * ti_nodes_node_by_id(uint8_t node_id);
ti_node_t * ti_nodes_random_ready_node(void);

struct ti_nodes_s
{
    vec_t * vec;
    uv_tcp_t tcp;
    struct sockaddr_storage addr;
};

#endif /* TI_NODES_H_ */
