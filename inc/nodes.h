/*
 * nodes.h
 */
#ifndef THINGSDB_NODES_H_
#define THINGSDB_NODES_H_

#include <uv.h>
#include <ti/node.h>
#include <util/vec.h>
#include <qpack.h>

typedef struct thingsdb_nodes_s thingsdb_nodes_t;

int thingsdb_nodes_create(void);
void thingsdb_nodes_destroy(void);
int thingsdb_nodes_listen(void);
_Bool thingsdb_nodes_has_quorum(void);
int thingsdb_nodes_to_packer(qp_packer_t ** packer);
int thingsdb_nodes_from_qpres(qp_res_t * qpnodes);
ti_node_t * thingsdb_nodes_create_node(struct sockaddr_storage * addr);
ti_node_t * thingsdb_nodes_node_by_id(uint8_t * node_id);

struct thingsdb_nodes_s
{
    vec_t * vec;
    uv_tcp_t tcp;
    struct sockaddr_storage addr;
};

#endif /* THINGSDB_NODES_H_ */
