/*
 * node.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <string.h>
#include <stdlib.h>
#include <rql/node.h>

rql_node_t * rql_node_create(uint8_t id, char * addr, uint16_t port)
{
    rql_node_t * node = (rql_node_t *) malloc(sizeof(rql_node_t));
    if (!node) return NULL;

    node->ref = 1;
    node->id = id;
    node->port =port;
    node->sock = NULL;
    node->addr = strdup(addr);

    if (!node->addr)
    {
        rql_node_drop(node);
        return NULL;
    }

    return node;
}

rql_node_t * rql_node_grab(rql_node_t * node)
{
    node->ref++;
    return node;
}

void rql_node_drop(rql_node_t * node)
{
    if (node && !--node->ref)
    {
        rql_sock_drop(node->sock);
        free(node->addr);
        free(node);
    }
}
