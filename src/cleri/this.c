/*
 * this.c - cleri THIS element. there should be only one single instance
 *          of this which can even be shared over different grammars.
 *          Always use this element using its constant CLERI_THIS and
 *          somewhere within a prio element.
 */
#include <cleri/expecting.h>
#include <cleri/this.h>
#include <stdlib.h>
#include <assert.h>

static cleri_node_t * cleri_parse_this(
        cleri_parse_t * pr,
        cleri_node_t * parent,
        cleri_t * cl_obj,
        cleri_rule_store_t * rule);

static int cleri_dummy = 0;

static cleri_t cleri_this = {
        .gid=0,
        .ref=1,
        .free_object=NULL,
        .parse_object=&cleri_parse_this,
        .tp=CLERI_TP_THIS,
        .via={.dummy=(void *) &cleri_dummy}};

cleri_t * CLERI_THIS = &cleri_this;

/*
 * Returns a node or NULL. In case of an error pr->is_valid is set to -1.
 */
static cleri_node_t * cleri_parse_this(
        cleri_parse_t * pr,
        cleri_node_t * parent,
        cleri_t * cl_obj,
        cleri_rule_store_t * rule)
{
    cleri_node_t * node;
    cleri_rule_tested_t * tested;
    const char * str = parent->str + parent->len;

    switch (cleri__rule_init(&tested, &rule->tested, str))
    {
    case CLERI_RULE_TRUE:
        if (pr->flags & CLERI_FLAG_EXCLUDE_RULE_THIS)
        {
            tested->node = cleri__parse_walk(
                    pr,
                    parent,
                    rule->root_obj,
                    rule,
                    CLERI__EXP_MODE_REQUIRED);

            return tested->node == NULL ? NULL : parent;
        }
        if ((node = cleri__node_new(cl_obj, str, 0)) == NULL)
        {
            pr->is_valid = -1;
            return NULL;
        }
        assert(tested->node == NULL);
        tested->node = cleri__parse_walk(
                pr,
                node,
                rule->root_obj,
                rule,
                CLERI__EXP_MODE_REQUIRED);
        if (tested->node == NULL)
        {
            cleri__node_free(node);
            return NULL;
        }
        break;
    case CLERI_RULE_FALSE:
        node = tested->node;
        if (node == NULL)
        {
            return NULL;
        }
        if (node->next)
        {
            /* It is only required to duplicate when a sibling is set.
             * If no sibling is set, then either this node will win over the
             * previous parent but then the sibling will be kept, or the parent
             * will win, but then we should end up with no sibling
             */
            node = cleri__node_dup(node);
            if (node == NULL)
            {
                pr->is_valid = -1;
                return NULL;
            }
        }
        else
        {
            node->ref++;
        }
        break;
    case CLERI_RULE_ERROR:
        pr->is_valid = -1;
        return NULL;

    default:
        assert (0);
        return NULL;
    }

    parent->len += node->len;
    cleri__node_add(parent, node);
    return node;
}
