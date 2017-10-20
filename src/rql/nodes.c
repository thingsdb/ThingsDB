/*
 * nodes.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <rql/nodes.h>
#include <rql/node.h>

_Bool rql_nodes_has_quorum(vec_t * nodes)
{
    size_t quorum = (nodes->n + 1) / 2;
    size_t q = 0;

    for (vec_each(nodes, rql_node_t, node))
    {
        if (node->status > RQL_NODE_STAT_CONNECTED && ++q == quorum) return 1;
    }

    return 0;
}
