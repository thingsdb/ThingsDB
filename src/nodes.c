/*
 * nodes.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <nodes.h>
#include <thingsdb.h>
#include <ti/node.h>
#include <util/vec.h>

static vec_t ** nodes;

int thingsdb_nodes_init(void)
{
    nodes = &(thingsdb_get()->nodes = vec_new(0));
    return -(*nodes == NULL);
}

void thingsdb_nodes_destroy(void)
{
    vec_destroy(*nodes, (vec_destroy_cb) ti_node_drop);
    *nodes = NULL;
}

_Bool ti_nodes_has_quorum(void)
{
    size_t quorum = ((*nodes)->n + 1) / 2;
    size_t q = 0;

    for (vec_each(*nodes, ti_node_t, node))
    {
        if (node->status > TI_NODE_STAT_CONNECTED && ++q == quorum) return 1;
    }

    return 0;
}
