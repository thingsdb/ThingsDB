/*
 * node.c
 */
#include <string.h>
#include <stdlib.h>
#include <ti/node.h>
#include <ti/write.h>

static void ti__node_write_cb(ti_write_t * req, ex_e status);

/*
 * Nodes are created ti_nodes_create_node() to ensure a correct id
 * is generated for each node.
 */
ti_node_t * ti_node_create(uint8_t id, struct sockaddr_storage * addr)
{
    ti_node_t * node = (ti_node_t *) malloc(sizeof(ti_node_t));
    if (!node)
        return NULL;

    node->ref = 1;
    node->maintn = 0;
    node->id = id;
    node->stream = NULL;
    node->status = TI_NODE_STAT_OFFLINE;
    node->addr = *addr;

    return node;
}

void ti_node_drop(ti_node_t * node)
{
    if (node && !--node->ref)
    {
        ti_stream_drop(node->stream);
        free(node);
    }
}

int ti_node_write(ti_node_t * node, ti_pkg_t * pkg)
{
    return ti_write(node->stream, pkg, NULL, ti__node_write_cb);
}

static void ti__node_write_cb(ti_write_t * req, ex_e status)
{
    (void)(status);
    free(req->pkg);
    ti_write_destroy(req);
}
