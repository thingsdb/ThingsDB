/*
 * list.c - cleri list element.
 */
#include <cleri/list.h>
#include <stdlib.h>

static void list__free(cleri_t * cl_object);
static cleri_node_t * list__parse(
        cleri_parse_t * pr,
        cleri_node_t * parent,
        cleri_t * cl_obj,
        cleri_rule_store_t * rule);

/*
 * Returns NULL in case an error has occurred.
 *
 * cl_obj       :   object to repeat
 * delimiter    :   object (Usually a Token) as delimiter
 * min          :   should be equal to or higher then 0.
 * max          :   should be equal to or higher then 0 but when 0 it
 *                  means unlimited.
 * opt_closing  :   when set to true (1) the list can be closed with a
 *                  delimiter. when false (0) this is not allowed.
 */
cleri_t * cleri_list(
        uint32_t gid,
        cleri_t * cl_obj,
        cleri_t * delimiter,
        size_t min,
        size_t max,
        int opt_closing)
{
    if (cl_obj == NULL || delimiter == NULL)
    {
        return NULL;
    }

    cleri_t * cl_object = cleri_new(
            gid,
            CLERI_TP_LIST,
            &list__free,
            &list__parse);

    if (cl_object == NULL)
    {
        return NULL;
    }

    cl_object->via.list = cleri__malloc(cleri_list_t);

    if (cl_object->via.list == NULL)
    {
        free(cl_object);
        return NULL;
    }

    cl_object->via.list->cl_obj = cl_obj;
    cl_object->via.list->delimiter = delimiter;
    cl_object->via.list->min = min;
    cl_object->via.list->max = max;
    cl_object->via.list->opt_closing = opt_closing;

    cleri_incref(cl_obj);
    cleri_incref(delimiter);

    return cl_object;
}

/*
 * Destroy list object.
 */
static void list__free(cleri_t * cl_object)
{
    cleri_free(cl_object->via.list->cl_obj);
    cleri_free(cl_object->via.list->delimiter);
    free(cl_object->via.list);
}

/*
 * Returns a node or NULL. In case of an error pr->is_valid is set to -1.
 */
static cleri_node_t * list__parse(
        cleri_parse_t * pr,
        cleri_node_t * parent,
        cleri_t * cl_obj,
        cleri_rule_store_t * rule)
{
    cleri_list_t * list = cl_obj->via.list;
    cleri_node_t * node;
    cleri_node_t * rnode;
    size_t i = 0;
    size_t j = 0;

    if ((node = cleri__node_new(cl_obj, parent->str + parent->len, 0)) == NULL)
    {
        pr->is_valid = -1;
        return NULL;
    }

    while (1)
    {
        rnode = cleri__parse_walk(
                pr,
                node,
                list->cl_obj,
                rule,
                i < list->min); // 1 = REQUIRED
        if (rnode == NULL || (++i == list->max && list->opt_closing == false))
        {
            break;
        }
        rnode = cleri__parse_walk(
                pr,
                node,
                list->delimiter,
                rule,
                i < list->min); // 1 = REQUIRED
        if (rnode == NULL || ++j == list->max)
        {
            break;
        }
    }
    if (i < list->min || (list->opt_closing == false && i && i == j))
    {
        cleri__node_free(node);
        return NULL;
    }
    parent->len += node->len;
    cleri__node_add(parent, node);
    return node;
}
