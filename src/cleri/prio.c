/*
 * prio.c - cleri prio element. (this element create a cleri rule object
 *          holding this prio element)
 */
#include <cleri/prio.h>
#include <cleri/expecting.h>
#include <cleri/olist.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

static void prio__free(cleri_t * cl_obj);

static cleri_node_t *  prio__parse(
        cleri_parse_t * pr,
        cleri_node_t * parent,
        cleri_t * cl_obj,
        cleri_rule_store_t * rule);

/*
 * Returns NULL in case an error has occurred.
 */
cleri_t * cleri_prio(uint32_t gid, size_t len, ...)
{
    va_list ap;
    cleri_t * cl_object = cleri_new(
            0,
            CLERI_TP_PRIO,
            &prio__free,
            &prio__parse);

    if (cl_object == NULL)
    {
        return NULL;
    }

    cl_object->via.prio = cleri__malloc(cleri_prio_t);

    if (cl_object->via.prio == NULL)
    {
        free(cl_object);
        return NULL;
    }

    cl_object->via.prio->olist = cleri__olist_new();

    if (cl_object->via.prio->olist == NULL)
    {
        cleri_free(cl_object);
        return NULL;
    }

    va_start(ap, len);
    while(len--)
    {
        if (cleri__olist_append(
                cl_object->via.prio->olist,
                va_arg(ap, cleri_t *)))
        {
            cleri__olist_cancel(cl_object->via.prio->olist);
            cleri_free(cl_object);
            cl_object = NULL;
        }
    }
    va_end(ap);

    return cleri__rule(gid, cl_object);
}

/*
 * Destroy prio object.
 */
static void prio__free(cleri_t * cl_object)
{
    cleri__olist_free(cl_object->via.prio->olist);
    free(cl_object->via.prio);
}

/*
 * Returns a node or NULL. In case of an error pr->is_valid is set to -1.
 */
static cleri_node_t *  prio__parse(
        cleri_parse_t * pr,
        cleri_node_t * parent,
        cleri_t * cl_obj,
        cleri_rule_store_t * rule)
{
    cleri_olist_t * olist;
    cleri_node_t * node;
    cleri_node_t * rnode;
    cleri_rule_tested_t * tested;
    const char * str = parent->str + parent->len;

    /* initialize and return rule test, or return an existing test
     * if *str is already in tested */
    if (    rule->depth++ > MAX_RECURSION_DEPTH ||
            cleri__rule_init(&tested, &rule->tested, str) == CLERI_RULE_ERROR)
    {
        pr->is_valid = -1;
        return NULL;
    }

    olist = cl_obj->via.prio->olist;

    while (olist != NULL)
    {
        if ((node = cleri__node_new(cl_obj, str, 0)) == NULL)
        {
            pr->is_valid = -1;
            return NULL;
        }
        rnode = cleri__parse_walk(
                pr,
                node,
                olist->cl_obj,
                rule,
                CLERI__EXP_MODE_REQUIRED);

        if (rnode != NULL &&
                (tested->node == NULL || node->len > tested->node->len))
        {
            if (tested->node != NULL)
            {
                /*
                 * It is required to decrement an extra reference here, one
                 * belongs to the parse result, and one for the tested rule.
                 * The node->ref increment below is required for when a str
                 * position is visited a second time by another parent.
                 */
                --tested->node->ref;
                cleri__node_free(tested->node);
            }
            tested->node = node;
            node->ref++;
        }
        else
        {
            cleri__node_free(node);
        }
        olist = olist->next;
    }

    rule->depth--;

    if (tested->node != NULL)
    {
        parent->len += tested->node->len;
        cleri__node_add(parent, tested->node);
        return tested->node;
    }
    return NULL;
}

