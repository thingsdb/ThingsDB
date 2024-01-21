/*
 * node.c - node is created while parsing a string. a node old the result
 *          for one element.
 */
#include <cleri/node.h>
#include <stdlib.h>

static cleri_node_t CLERI__EMPTY_NODE = {
        .children=NULL,
        .next=NULL,
        .cl_obj=NULL,
        .len=0,
        .str=NULL,
        .ref=1,
};

cleri_node_t * CLERI_EMPTY_NODE = &CLERI__EMPTY_NODE;

/*
 * Returns NULL in case an error has occurred.
 */
cleri_node_t * cleri__node_new(cleri_t * cl_obj, const char * str, size_t len)
{
    cleri_node_t * node = cleri__malloc(cleri_node_t);

    if (node != NULL)
    {
        node->cl_obj = cl_obj;
        node->ref = 1;

        node->str = str;
        node->len = len;
        node->children = NULL;
        node->next = NULL;
        node->data = &node->children;
    }
    return node;
}

cleri_node_t * cleri__node_dup(cleri_node_t * node)
{
    cleri_node_t * dup = cleri__malloc(cleri_node_t);
    if (dup != NULL)
    {
        dup->cl_obj = node->cl_obj;
        dup->ref = 1;

        dup->str = node->str;
        dup->len = node->len;

        node = node->children;

        dup->children = node;

        while(node)
        {
            node->ref++;
            node = node->next;
        }

        dup->next = NULL;
    }
    return dup;
}

void cleri__node_free(cleri_node_t * node)
{
    cleri_node_t * next;

    /* node can be NULL or this could be an CLERI_EMPTY_NODE */
    if (node == NULL || --node->ref)
    {
        return;
    }

    next = node->children;

    while (next != NULL)
    {
        cleri_node_t * tmp = next->next;
        next->next = NULL;
        cleri__node_free(next);
        next = tmp;
    }

    free(node);
}
