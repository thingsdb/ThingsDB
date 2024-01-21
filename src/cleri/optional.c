/*
 * optional.c - cleri optional element.
 */
#include <cleri/optional.h>
#include <cleri/expecting.h>
#include <stdlib.h>

static void optional__free(cleri_t * cl_object);

static cleri_node_t * optional__parse(
        cleri_parse_t * pr,
        cleri_node_t * parent,
        cleri_t * cl_obj,
        cleri_rule_store_t * rule);

/*
 * Returns NULL and in case an error has occurred.
 */
cleri_t * cleri_optional(uint32_t gid, cleri_t * cl_obj)
{
    if (cl_obj == NULL)
    {
        return NULL;
    }

    cleri_t * cl_object = cleri_new(
            gid,
            CLERI_TP_OPTIONAL,
            &optional__free,
            &optional__parse);

    if (cl_object == NULL)
    {
        return NULL;
    }

    cl_object->via.optional = cleri__malloc(cleri_optional_t);

    if (cl_object->via.optional == NULL)
    {
        free(cl_object);
        return NULL;
    }

    cl_object->via.optional->cl_obj = cl_obj;

    cleri_incref(cl_obj);

    return cl_object;
}

/*
 * Destroy optional object.
 */
static void optional__free(cleri_t * cl_object)
{
    cleri_free(cl_object->via.optional->cl_obj);
    free(cl_object->via.optional);
}

/*
 * Returns a node or NULL. In case of an error pr->is_valid is set to -1.
 */
static cleri_node_t * optional__parse(
        cleri_parse_t * pr,
        cleri_node_t * parent,
        cleri_t * cl_obj,
        cleri_rule_store_t * rule)
{
    cleri_node_t * node;
    cleri_node_t * rnode;

    if ((pr->flags & CLERI_FLAG_EXCLUDE_OPTIONAL) && !cl_obj->gid)
    {
        node = cleri__parse_walk(
            pr,
            parent,
            cl_obj->via.optional->cl_obj,
            rule,
            CLERI__EXP_MODE_OPTIONAL);
        return node ? node : CLERI_EMPTY_NODE;
    }

    if ((node = cleri__node_new(cl_obj, parent->str + parent->len, 0)) == NULL)
    {
        pr->is_valid = -1;
        return NULL;
    }

    rnode = cleri__parse_walk(
            pr,
            node,
            cl_obj->via.optional->cl_obj,
            rule,
            CLERI__EXP_MODE_OPTIONAL);

    if (rnode != NULL)
    {
        parent->len += node->len;
        cleri__node_add(parent, node);
        return node;
    }

    cleri__node_free(node);
    return CLERI_EMPTY_NODE;
}
