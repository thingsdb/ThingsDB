/*
 * node.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_NODE_H_
#define RQL_NODE_H_

typedef struct rql_node_s  rql_node_t;

#include <inttypes.h>
#include <rql/sock.h>

struct rql_node_s
{
    uint64_t ref;
    uint8_t id;  /* equal to the index in rql->nodes */
    uint8_t flags;
    uint16_t port;
    imap_t * requests;
    rql_sock_t * sock;
    char * addr;
};

rql_node_t * rql_node_create(uint8_t id, char * address, uint16_t port);
rql_node_t * rql_node_grab(rql_node_t * node);
void rql_node_drop(rql_node_t * node);
//int rql_node_write(rql_node_t * node, rql_pkg_t * pkg, rql_write_cb cb);

#endif /* RQL_NODE_H_ */
