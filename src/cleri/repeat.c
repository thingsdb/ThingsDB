/*
 * repeat.c - cleri regular repeat element.
 */
#include <cleri/repeat.h>
#include <stdlib.h>
#include <assert.h>

static void repeat__free(cleri_t * cl_object);

static cleri_node_t * repeat__parse(
        cleri_parse_t * pr,
        cleri_node_t * parent,
        cleri_t * cl_obj,
        cleri_rule_store_t * rule);

/*
 * Returns NULL in case an error has occurred.
 *
 * cl_ob :      object to repeat
 * min :        should be equal to or higher then 0.
 * max :        should be equal to or higher then min, or 0 which means
 *              unlimited.
 */
cleri_t * cleri_repeat(uint32_t gid, cleri_t * cl_obj, size_t min, size_t max)
{
    if (cl_obj == NULL)
    {
        return NULL;
    }

    assert (!max || max >= min);

    cleri_t * cl_object = cleri_new(
            gid,
            CLERI_TP_REPEAT,
            &repeat__free,
            &repeat__parse);

    if (cl_object == NULL)
    {
        return NULL;
    }

    cl_object->via.repeat = cleri__malloc(cleri_repeat_t);

    if (cl_object->via.repeat == NULL)
    {
        free(cl_object);
        return NULL;
    }

    cl_object->via.repeat->cl_obj = cl_obj;
    cl_object->via.repeat->min = min;
    cl_object->via.repeat->max = max;

    cleri_incref(cl_obj);

    return cl_object;
}

/*
 * Destroy repeat object.
 */
static void repeat__free(cleri_t * cl_object)
{
    cleri_free(cl_object->via.repeat->cl_obj);
    free(cl_object->via.repeat);
}

/*
 * Returns a node or NULL. In case of an error pr->is_valid is set to -1.
 */
static cleri_node_t * repeat__parse(
        cleri_parse_t * pr,
        cleri_node_t * parent,
        cleri_t * cl_obj,
        cleri_rule_store_t * rule)
{
    cleri_node_t * node;
    cleri_node_t * rnode;
    size_t i;
    if ((node = cleri__node_new(cl_obj, parent->str + parent->len, 0)) == NULL)
    {
        pr->is_valid = -1;
        return NULL;
    }

    for (i = 0;
         cl_obj->via.repeat->max == 0 || i < cl_obj->via.repeat->max;
         i++)
    {
        rnode = cleri__parse_walk(
                pr,
                node,
                cl_obj->via.repeat->cl_obj,
                rule,
                i < cl_obj->via.repeat->min); // 1 = REQUIRED
        if (rnode == NULL)
        {
            break;
        }
    }

    if (i < cl_obj->via.repeat->min)
    {
        cleri__node_free(node);
        return NULL;
    }
    parent->len += node->len;
    cleri__node_add(parent, node);
    return node;
}
