/*
 * nodes.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <rql/nodes.h>
#include <rql/node.h>

_Bool rql_nodes_has_quorum(rql_t * rql)
{
    size_t quorum = rql->nodes->n / 2;
    size_t q = 0;

    for (vec_each(rql->nodes, rql_node_t, node))
    {
        if (node->status > RQL_NODE_STAT_CONNECTED && ++q == quorum) return 1;
    }

    return 0;
}
