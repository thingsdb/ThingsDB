/*
 * node.h - node is created while parsing a string. a node old the result
 *          for one element.
 */
#ifndef CLERI_NODE_H_
#define CLERI_NODE_H_

#include <stddef.h>
#include <inttypes.h>
#include <cleri/cleri.h>
#include <cleri/node.h>

/* typedefs */
typedef struct cleri_s cleri_t;
typedef struct cleri_node_s cleri_node_t;

/* public macro function */
#define cleri_node_has_children(__node) ((__node)->children != NULL)

/* private functions */
cleri_node_t * cleri__node_new(cleri_t * cl_obj, const char * str, size_t len);
cleri_node_t * cleri__node_dup(cleri_node_t * node);
void cleri__node_free(cleri_node_t * node);

/* private use as empty node */
extern cleri_node_t * CLERI_EMPTY_NODE;

/* structs */
struct cleri_node_s
{
    uint32_t ref;
    uint32_t len;

    const char * str;
    cleri_t * cl_obj;
    cleri_node_t * children;
    cleri_node_t * next;

    void * data;        /* free to use by the user */
};

static inline void cleri__node_add(cleri_node_t * parent, cleri_node_t * node)
{
    cleri_node_t ** nodeaddr = parent->data;
    *nodeaddr = node;
    parent->data = &node->next;
}

#endif /* CLERI_NODE_H_ */