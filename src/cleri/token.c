/*
 * token.c - cleri token element. note that one single char will parse
 *           slightly faster compared to tokens containing more characters.
 *           (be careful a token should not match the keyword regular
 *           expression)
 */
#include <cleri/token.h>
#include <stdlib.h>
#include <string.h>

static void token__free(cleri_t * cl_object);
static cleri_node_t * token__parse(
        cleri_parse_t * pr,
        cleri_node_t * parent,
        cleri_t * cl_obj,
        cleri_rule_store_t * rule);

/*
 * Returns NULL in case an error has occurred.
 */
cleri_t * cleri_token(uint32_t gid, const char * token)
{
    cleri_t * cl_object = cleri_new(
            gid,
            CLERI_TP_TOKEN,
            &token__free,
            &token__parse);


    if (cl_object == NULL)
    {
        return NULL;
    }

    cl_object->via.token = cleri__malloc(cleri_token_t);

    if (cl_object->via.token == NULL)
    {
        free(cl_object);
        return NULL;
    }

    cl_object->via.token->token = token;
    cl_object->via.token->len = strlen(token);

    return cl_object;
}

/*
 * Destroy token object.
 */
static void token__free(cleri_t * cl_object)
{
    free(cl_object->via.token);
}

/*
 * Returns a node or NULL. In case of an error pr->is_valid is set to -1.
 */
static cleri_node_t * token__parse(
        cleri_parse_t * pr,
        cleri_node_t * parent,
        cleri_t * cl_obj,
        cleri_rule_store_t * rule __attribute__((unused)))
{
    cleri_node_t * node = NULL;
    const char * str = parent->str + parent->len;
    if (strncmp(
            cl_obj->via.token->token,
            str,
            cl_obj->via.token->len) == 0)
    {
        if ((node = cleri__node_new(
                cl_obj,
                str,
                cl_obj->via.token->len)) != NULL)
        {
            parent->len += node->len;
            cleri__node_add(parent, node);
        }
        else
        {
            pr->is_valid = -1;
        }
    }
    else if (cleri__expecting_update(pr->expecting, cl_obj, str) == -1)
    {
        pr->is_valid = -1;
    }
    return node;
}

