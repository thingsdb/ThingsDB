/*
 * tokens.c - cleri tokens element. (like token but can contain more tokens
 *            in one element)
 */
#include <cleri/tokens.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

static void tokens__free(cleri_t * cl_object);
static cleri_node_t * tokens__parse(
        cleri_parse_t * pr,
        cleri_node_t * parent,
        cleri_t * cl_obj,
        cleri_rule_store_t * rule);
static int tokens__list_add(
        cleri_tlist_t ** tlist,
        const char * token,
        size_t len);
static void tokens__list_str(cleri_tlist_t * tlist, char * s);
static void tokens__list_free(cleri_tlist_t * tlist);

/*
 * Returns NULL in case an error has occurred.
 */
cleri_t * cleri_tokens(uint32_t gid, const char * tokens)
{
    size_t len;
    char * pt;
    cleri_t * cl_object;

    cl_object = cleri_new(
            gid,
            CLERI_TP_TOKENS,
            &tokens__free,
            &tokens__parse);

    if (cl_object == NULL)
    {
        return NULL;
    }

    cl_object->via.tokens = cleri__malloc(cleri_tokens_t);

    if (cl_object->via.tokens == NULL)
    {
        free(cl_object);
        return NULL;
    }

    /* copy the sting twice, first one we set spaces to 0...*/
    cl_object->via.tokens->tokens = strdup(tokens);

    /* ...and this one we keep for showing a spaced version */
    cl_object->via.tokens->spaced = (char *) malloc(strlen(tokens) + 1);

    cl_object->via.tokens->tlist = cleri__malloc(cleri_tlist_t);


    if (    cl_object->via.tokens->tokens == NULL ||
            cl_object->via.tokens->spaced == NULL ||
            cl_object->via.tokens->tlist == NULL)
    {
        cleri_free(cl_object);
        return NULL;
    }

    cl_object->via.tokens->tlist->token = NULL;
    cl_object->via.tokens->tlist->next = NULL;
    cl_object->via.tokens->tlist->len = 0;

    pt = cl_object->via.tokens->tokens;

    for (len = 0;; pt++)
    {
        if (!*pt || isspace(*pt))
        {
            if (len)
            {
                if (tokens__list_add(
                        &cl_object->via.tokens->tlist,
                        pt - len,
                        len))
                {
                    cleri_free(cl_object);
                    return NULL;
                }
                len = 0;
            }
            if (*pt)
                *pt = 0;
            else
            {
                break;
            }
        }
        else
        {
            len++;
        }
    }

    /* tlist->token is never empty */
    assert (cl_object->via.tokens->tlist->token != NULL);

    tokens__list_str(
        cl_object->via.tokens->tlist,
        cl_object->via.tokens->spaced);

    return cl_object;
}

/*
 * Destroy token object.
 */
static void tokens__free(cleri_t * cl_object)
{
    tokens__list_free(cl_object->via.tokens->tlist);
    free(cl_object->via.tokens->tokens);
    free(cl_object->via.tokens->spaced);
    free(cl_object->via.tokens);
}

/*
 * Returns a node or NULL. In case of an error pr->is_valid is set to -1.
 */
static cleri_node_t * tokens__parse(
        cleri_parse_t * pr,
        cleri_node_t * parent,
        cleri_t * cl_obj,
        cleri_rule_store_t * rule __attribute__((unused)))
{
    cleri_node_t * node = NULL;
    const char * str = parent->str + parent->len;
    cleri_tlist_t * tlist = cl_obj->via.tokens->tlist;

    /* we can trust that at least one token is in the list */
    for (; tlist != NULL; tlist = tlist->next)
    {
        if (strncmp(tlist->token, str, tlist->len) == 0)
        {
            if ((node = cleri__node_new(cl_obj, str, tlist->len)) != NULL)
            {
                parent->len += node->len;
                cleri__node_add(parent, node);
            }
            else
            {
                pr->is_valid = -1;
            }
            return node;
        }
    }
    if (cleri__expecting_update(pr->expecting, cl_obj, str) == -1)
    {
        pr->is_valid = -1;
    }
    return NULL;
}

/*
 * Returns 0 if successful and -1 in case an error occurred.
 * (the token list remains unchanged in case of an error)
 *
 * The token will be placed in the list based on the length. Thus the token
 * list remains ordered on length. (largest first)
 */
static int tokens__list_add(
        cleri_tlist_t ** tlist,
        const char * token,
        size_t len)
{
    cleri_tlist_t * tmp, * prev, * current;
    current = prev = *tlist;

    if (current->token == NULL)
    {
        current->token = token;
        current->len = len;
        return 0;
    }
    tmp = cleri__malloc(cleri_tlist_t);
    if (tmp == NULL)
    {
        return -1;
    }
    tmp->len = len;
    tmp->token = token;

    while (current != NULL && len <= current->len)
    {
        prev = current;
        current = current->next;
    }

    if (current == *tlist)
    {
        tmp->next = *tlist;
        *tlist = tmp;
    }
    else
    {
        tmp->next = current;
        prev->next = tmp;
    }

    return 0;
}

/*
 * Copies tokens to spaced. This is the input tokes orders by their length.
 * No errors can occur since in any case enough space is allocated for *s.
 */
static void tokens__list_str(cleri_tlist_t * tlist, char * s)
{
    assert (tlist->token != NULL);
    memcpy(s, tlist->token, tlist->len);
    s += tlist->len;
    while ((tlist = tlist->next) != NULL)
    {
        *s = ' ';
        s++;
        memcpy(s, tlist->token, tlist->len);
        s += tlist->len;
    }
    *s = '\0';
}

/*
 * Destroy token list. (NULL can be parsed tlist argument)
 */
static void tokens__list_free(cleri_tlist_t * tlist)
{
    cleri_tlist_t * next;
    while (tlist != NULL)
    {
        next = tlist->next;
        free(tlist);
        tlist = next;
    }
}

