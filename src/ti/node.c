/*
 * node.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <string.h>
#include <stdlib.h>
#include <ti/node.h>
#include <ti/write.h>

static void ti__node_write_cb(ti_write_t * req, ex_e status);

ti_node_t * ti_node_create(uint8_t id, char * addr, uint16_t port)
{
    ti_node_t * node = (ti_node_t *) malloc(sizeof(ti_node_t));
    if (!node) return NULL;

    node->ref = 1;
    node->maintn = 0;
    node->id = id;
    node->port =port;
    node->sock = NULL;
    node->addr = strdup(addr);
    node->status = TI_NODE_STAT_OFFLINE;

    if (!node->addr)
    {
        ti_node_drop(node);
        return NULL;
    }

    return node;
}

ti_node_t * ti_node_grab(ti_node_t * node)
{
    node->ref++;
    return node;
}

void ti_node_drop(ti_node_t * node)
{
    if (node && !--node->ref)
    {
        ti_sock_drop(node->sock);
        free(node->addr);
        free(node);
    }
}

int ti_node_write(ti_node_t * node, ti_pkg_t * pkg)
{
    return ti_write(node->sock, pkg, NULL, ti__node_write_cb);
}

static void ti__node_write_cb(ti_write_t * req, ex_e status)
{
    (void)(status);
    free(req->pkg);
    ti_write_destroy(req);
}
