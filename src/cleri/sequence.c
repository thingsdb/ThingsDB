/*
 * sequence.c - cleri sequence element.
 */
#include <cleri/sequence.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

static void sequence__free(cleri_t * cl_object);
static cleri_node_t * sequence__parse(
        cleri_parse_t * pr,
        cleri_node_t * parent,
        cleri_t * cl_obj,
        cleri_rule_store_t * rule);

/*
 * Returns NULL and in case an error has occurred.
 */
cleri_t * cleri_sequence(uint32_t gid, size_t len, ...)
{
    va_list ap;
    cleri_t * cl_object = cleri_new(
            gid,
            CLERI_TP_SEQUENCE,
            &sequence__free,
            &sequence__parse);

    if (cl_object == NULL)
    {
        return NULL;
    }

    cl_object->via.sequence = cleri__malloc(cleri_sequence_t);

    if (cl_object->via.sequence == NULL)
    {
        free(cl_object);
        return NULL;
    }

    cl_object->via.sequence->olist = cleri__olist_new();

    if (cl_object->via.sequence->olist == NULL)
    {
        cleri_free(cl_object);
        return NULL;
    }

    va_start(ap, len);
    while(len--)
    {
        if (cleri__olist_append(
                cl_object->via.sequence->olist,
                va_arg(ap, cleri_t *)))
        {
            cleri__olist_cancel(cl_object->via.sequence->olist);
            cleri_free(cl_object);
            cl_object = NULL;
            break;
        }
    }
    va_end(ap);

    return cl_object;
}

/*
 * Destroy sequence object.
 */
static void sequence__free(cleri_t * cl_object)
{
    cleri__olist_free(cl_object->via.sequence->olist);
    free(cl_object->via.sequence);
}

/*
 * Returns a node or NULL. In case of an error pr->is_valid is set to -1.
 */
static cleri_node_t * sequence__parse(
        cleri_parse_t * pr,
        cleri_node_t * parent,
        cleri_t * cl_obj,
        cleri_rule_store_t * rule)
{
    cleri_olist_t * olist;
    cleri_node_t * node;
    cleri_node_t * rnode;

    olist = cl_obj->via.sequence->olist;
    if ((node = cleri__node_new(cl_obj, parent->str + parent->len, 0)) == NULL)
    {
        pr->is_valid = -1;
        return NULL;
    }

    while (olist != NULL)
    {
        rnode = cleri__parse_walk(
                pr,
                node,
                olist->cl_obj,
                rule,
                CLERI__EXP_MODE_REQUIRED);
        if (rnode == NULL)
        {
            cleri__node_free(node);
            return NULL;
        }
        olist = olist->next;
    }

    parent->len += node->len;
    cleri__node_add(parent, node);
    return node;
}
