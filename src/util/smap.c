/*
 * smap.c
 */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdint.h>
#include <util/smap.h>
#include <util/logger.h>

#define SMAP_BSZ 32

static smap_node_t * smap__node_create(
        const char * key,
        size_t n,
        void * data);
static int smap__node_resize(smap_node_t * node, uint8_t pos);
static int smap__add(smap_node_t * node, const char * key, void * data);
static void * smap__pop(
        smap_node_t * parent,
        smap_node_t ** nd,
        const char * key);
static void smap__dec_node(smap_node_t * node);
static void smap__merge_node(smap_node_t * node);
static size_t smap__longest_key_size(smap_node_t * node);
static int smap__items(
        smap_node_t * node,
        size_t n,
        char * buf,
        smap_item_cb cb,
        void * arg);
static int smap__values(smap_node_t * node, smap_val_cb cb, void * arg);
static void smap__destroy(smap_node_t * node, smap_destroy_cb cb);

/*
 * Returns a new smap or NULL in case of an allocation error.
 */
smap_t * smap_create(void)
{
    smap_t * smap = malloc(sizeof(smap_t));
    if (!smap)
        return NULL;
    smap->n = 0;
    smap->nodes = NULL;
    smap->offset = UINT8_MAX;
    smap->sz = 0;
    return smap ;
}

/*
 * Destroy smap.
 */
void smap_destroy(smap_t * smap, smap_destroy_cb cb)
{
    if (!smap)
        return;

    if (smap->nodes)
    {
        for (uint_fast16_t i = 0, end = smap->sz * SMAP_BSZ; i < end; i++)
        {
            if ((*smap->nodes)[i])
            {
                smap__destroy((*smap->nodes)[i], cb);
            }
        }
        free(smap->nodes);
    }
    free(smap);
}

/*
 * Adds new key/data to the smap.
 *
 * Returns 0 when successful or SMAP_ERR_EXIST if the key already exists.
 *
 * When the key exists the value will not be overwritten.
 *
 * A key must have a length larger than zero, otherwise SMAP_ERR_EMPTY_KEY will
 * be returned.
 *
 * In case of an allocation error, SMAP_ERR_ALLOC will be returned.
 */
int smap_add(smap_t * smap, const char * key, void * data)
{
    int rc;
    smap_node_t ** nd;
    uint8_t k = (uint8_t) *key;
    if (!*key)
        return SMAP_ERR_EMPTY_KEY;

    if (smap__node_resize((smap_node_t *) smap, k / SMAP_BSZ))
        return SMAP_ERR_ALLOC;

    nd = &(*smap->nodes)[k - smap->offset * SMAP_BSZ];
    key++;

    if (*nd)
    {
        rc = smap__add(*nd, key, data);
        if (rc == 0)
        {
            smap->n++;
        }
    }
    else
    {
        *nd = smap__node_create(key, strlen(key), data);
        if (*nd)
        {
            smap->n++;
            rc = 0;
        }
        else
        {
            rc = SMAP_ERR_ALLOC;
        }
    }
    return rc;
}

/*
 * Returns an item or NULL if the key does not exist.
 */
void * smap_get(smap_t * smap, const char * key)
{
    void ** data = smap_getaddr(smap, key);
    return data ? *data : NULL;
}

/*
 * Returns the address of an item or NULL if the key does not exist.
 */
void ** smap_getaddr(smap_t * smap, const char * key)
{
    smap_node_t * nd;
    uint8_t k = (uint8_t) *key;
    uint8_t pos = k / SMAP_BSZ;

    if (!*key || pos < smap->offset || pos >= smap->offset + smap->sz)
        return NULL;

    nd = (*smap->nodes)[k - smap->offset * SMAP_BSZ];

    while (nd && !strncmp(nd->key, ++key, nd->n))
    {
        key += nd->n;

        if (!*key)
            return &nd->data;

        if (!nd->nodes)
            return NULL;

        k = (uint8_t) *key;
        pos = k / SMAP_BSZ;

        if (pos < nd->offset || pos >= nd->offset + nd->sz)
            return NULL;

        nd = (*nd->nodes)[k - nd->offset * SMAP_BSZ];
    }

    return NULL;
}

/*
 * Returns an item or NULL if the key does not exist.
 */
void * smap_getn(smap_t * smap, const char * key, size_t n)
{
    size_t diff = 1;
    smap_node_t * nd;
    uint8_t k, pos;

    if (!n)
        return NULL;

    k = (uint8_t) *key;
    pos = k / SMAP_BSZ;

    if (pos < smap->offset || pos >= smap->offset + smap->sz)
        return NULL;

    nd = (*smap->nodes)[k - smap->offset * SMAP_BSZ];

    while (nd)
    {
        key += diff;
        n -= diff;

        if (n < nd->n || memcmp(nd->key, key, nd->n))
            return NULL;

        if (nd->n == n)
            return nd->data;

        if (!nd->nodes)
            return NULL;

        k = (uint8_t) key[nd->n];
        pos = k / SMAP_BSZ;

        if (pos < nd->offset || pos >= nd->offset + nd->sz)
            return NULL;

        diff = nd->n + 1; /* n - diff is at least 0 */
        nd = (*nd->nodes)[k - nd->offset * SMAP_BSZ];
    }

    return NULL;
}

/*
 * Removes and returns an item from the tree or NULL when not found.
 *
 * (re-allocation might fail but this is not critical)
 */
void * smap_pop(smap_t * smap, const char * key)
{
    smap_node_t ** nd;
    void * data;
    uint8_t k = (uint8_t) *key;
    uint8_t pos = k / SMAP_BSZ;

    if (!*key || pos < smap->offset || pos >= smap->offset + smap->sz)
        return NULL;

    nd = &(*smap->nodes)[k - smap->offset * SMAP_BSZ];

    if (!*nd)
        return NULL;

    data = smap__pop(NULL, nd, key + 1);
    if (data && !--smap->n)
    {
        free(smap->nodes);
        smap->nodes = NULL;
        smap->sz = 0;
    }

    return data;
}

/*
 * Returns the longest key size in smap.
 */
size_t smap_longest_key_size(smap_t * smap)
{
    size_t n, m = 0;
    smap_node_t * nd;
    for (uint_fast16_t i = 0, end = smap->sz * SMAP_BSZ; i < end; i++)
    {
        if (!(nd = (*smap->nodes)[i]))
            continue;
        n = smap__longest_key_size(nd);
        m = (n > m) ? n : m;
    }
    return m;
}

/*
 * Loop over all items in the tree and perform the call-back on each item.
 *
 * Looping stops on the first call-back returning a non-zero value.
 *
 * The return value is either 0 when the callback is successful called on all
 * values or the value of the failed callback.
 *
 * Argument 'buf' should be a buffer able to contain at least the longest key
 * size. If the length is unknown, smap_longest_key_size() can be used to
 * get the current size of the longest key.
 */
int smap_items(smap_t * smap, char * buf, smap_item_cb cb, void * arg)
{
    int rc;
    smap_node_t * nd;

    for (uint_fast16_t i = 0, end = smap->sz * SMAP_BSZ; i < end; i++)
    {
        if (!(nd = (*smap->nodes)[i]))
            continue;
        *buf = (char) (i + smap->offset * SMAP_BSZ);
        if ((rc = smap__items(nd, 1, buf, cb, arg)))
            return rc;
    }

    return 0;
}

/*
 * Loop over values until a non zero value is returned.
 *
 * The return value is either 0 when the callback is successful called on all
 * values or the value of the failed callback.
 */
int smap_values(smap_t * smap, smap_val_cb cb, void * arg)
{
    int rc;
    smap_node_t * nd;

    for (uint_fast16_t i = 0, end = smap->sz * SMAP_BSZ; i < end; i++)
    {
        if (!(nd = (*smap->nodes)[i]))
            continue;
        if ((rc = smap__values(nd, cb, arg)))
            return rc;
    }

    return 0;
}

static size_t smap__longest_key_size(smap_node_t * node)
{
    size_t n, m = 0;
    smap_node_t * nd;
    if (node->nodes)
    {
        for (uint_fast16_t i = 0, end = node->sz * SMAP_BSZ; i < end; i++)
        {
            if (!(nd = (*node->nodes)[i]))
                continue;
            n = smap__longest_key_size(nd);
            m = (n > m) ? n : m;
        }
    }
    m += node->n + 1;
    return m;
}

static int smap__items(
        smap_node_t * node,
        size_t n,
        char * buf,
        smap_item_cb cb,
        void * arg)
{
    int rc;
    memcpy(buf + n, node->key, node->n);
    n += node->n;

    if (node->data && (rc = (*cb)(buf, n, node->data, arg)))
        return rc;

    if (node->nodes)
    {
        smap_node_t * nd;

        for (uint_fast16_t i = 0, end = node->sz * SMAP_BSZ; i < end; i++)
        {
            if (!(nd = (*node->nodes)[i]))
                continue;
            *(buf + n) = (char) (i + node->offset * SMAP_BSZ);
            if ((rc = smap__items(nd, n + 1, buf, cb, arg)))
                return rc;
        }
    }

    return 0;
}

static int smap__values(smap_node_t * node, smap_val_cb cb, void * arg)
{
    int rc;
    if (node->data && (rc = (*cb)(node->data, arg)))
        return rc;

    if (node->nodes)
    {
        smap_node_t * nd;
        for (uint_fast16_t i = 0, end = node->sz * SMAP_BSZ; i < end; i++)
        {
            if (!(nd = (*node->nodes)[i]))
                continue;
            if ((rc = smap__values(nd, cb, arg)))
                return rc;
        }
    }

    return 0;
}

static int smap__add(smap_node_t * node, const char * key, void * data)
{
    for (size_t n = 0; n < node->n; n++, key++)
    {
        char * pt = node->key + n;
        if (*key != *pt)
        {
            size_t new_sz;
            uint8_t k = (uint8_t) *pt;

            /* create new nodes */
            smap_nodes_t * new_nodes = calloc(1, sizeof(smap_nodes_t));
            if (!new_nodes)
                return SMAP_ERR_ALLOC;

            /* create new nodes with rest of node pt */
            smap_node_t * nd = (*new_nodes)[k % SMAP_BSZ] =
                    smap__node_create(pt + 1, node->n - n - 1, node->data);
            if (!nd)
                return SMAP_ERR_ALLOC;

            /* bind the -rest- of current node to the new nodes */
            nd->nodes = node->nodes;
            nd->size = node->size;
            nd->offset = node->offset;
            nd->sz = node->sz;

            /* the current nodes should become the new nodes */
            node->nodes = new_nodes;
            node->offset = k / SMAP_BSZ;
            node->sz = 1;

            if (!*key)
            {
                node->size = 1;
                /* end of our key, store data in this node */
                node->data = data;
            }
            else
            {
                /* we have more, make sure data for this node is NULL and
                 * add rest of our key to the nodes.
                 */
                k = (uint8_t) *key;

                if (smap__node_resize(node, k / SMAP_BSZ))
                    return SMAP_ERR_ALLOC;
                key++;
                nd = smap__node_create(key, strlen(key), data);
                if (!nd)
                    return SMAP_ERR_ALLOC;

                node->size = 2;
                node->data = NULL;
                (*node->nodes)[k - node->offset * SMAP_BSZ] = nd;
            }

            /* re-allocate the key to free some space */
            if ((new_sz = pt - node->key))
            {
                char * tmp = realloc(node->key, new_sz);
                if (tmp)
                {
                    node->key = tmp;
                }
            }
            else
            {
                free(node->key);
                node->key = NULL;
            }
            node->n = new_sz;

            return 0;
        }
    }

    if (*key)
    {
        uint8_t k = (uint8_t) *key;

        if (!node->nodes)
        {
            if (smap__node_resize(node, k / SMAP_BSZ))
                return SMAP_ERR_ALLOC;  /* signal is raised */
            key++;
            smap_node_t * nd = smap__node_create(key, strlen(key), data);
            if (!nd)
                return SMAP_ERR_ALLOC;

            node->size = 1;
            (*node->nodes)[k - node->offset * SMAP_BSZ ] = nd;

            return 0;
        }

        if (smap__node_resize(node, k / SMAP_BSZ))
            return SMAP_ERR_ALLOC;

        smap_node_t ** nd = &(*node->nodes)[k - node->offset * SMAP_BSZ];
        key++;

        if (*nd)
            return smap__add(*nd, key, data);

        *nd = smap__node_create(key, strlen(key), data);
        if (!*nd)
            return SMAP_ERR_ALLOC;

        node->size++;
        return 0;
    }

    if (node->data)
        return SMAP_ERR_EXIST;

    node->data = data;

    return 0;
}

/*
 * Merge a child node with its parent for cleanup.
 *
 * This function should only be called when exactly one node is left and the
 * node itself has no data.
 *
 * In case re-allocation fails the tree remains unchanged and therefore
 * can still be used.
 */
static void smap__merge_node(smap_node_t * node)
{
    smap_node_t * child_node;
    char * tmp;
    uint_fast16_t i, end;

    for (i = 0, end = node->sz * SMAP_BSZ; i < end; i++)
    {
        if ((*node->nodes)[i])
            break;
    }
    /* this is the child node we need to merge */
    child_node = (*node->nodes)[i];

    /* re-allocate enough space for the key + child_key + 1 char */
    tmp = realloc(node->key, node->n + child_node->n + 1);
    if (!tmp)
        return;
    node->key = tmp;

    /* set node char */
    node->key[node->n++] = (char) (i + node->offset * SMAP_BSZ);

    /* append rest of the child key */
    memcpy(node->key + node->n, child_node->key, child_node->n);
    node->n += child_node->n;

    /* free nodes (has only the child node left so nothing else
     * needs cleaning */
    free(node->nodes);

    /* bind child nodes properties to the current node */
    node->nodes = child_node->nodes;
    node->size = child_node->size;
    node->offset = child_node->offset;
    node->sz = child_node->sz;
    node->data = child_node->data;

    /* free child key */
    free(child_node->key);

    /* free child node */
    free(child_node);
}

/*
 * A SIGNAL can be raised in case an error occurred. If this happens the node
 * size will still be decremented by one but the tree is not optionally merged.
 * This means that in case of an error its still possible to call
 * smap_destroy() or smap_destroy_cb() to destroy the tree.
 */
static void smap__dec_node(smap_node_t * node)
{
    if (!node) return;  /* this is the root node */

    node->size--;

    if (node->size == 0)
    {
        /* we can free nodes since they are no longer used */
        free(node->nodes);

        /* make sure to set nodes to NULL */
        node->nodes = NULL;
    }
    else if (node->size == 1 && !node->data)
    {
        smap__merge_node(node);
    }
}

/*
 * Removes and returns an item from the tree or NULL when not found.
 */
static void * smap__pop(
        smap_node_t * parent,
        smap_node_t ** nd,
        const char * key)
{
    smap_node_t * node = *nd;
    if (strncmp(node->key, key, node->n))
        return NULL;

    key += node->n;

    if (!*key)
    {
        void * data = node->data;
        if (node->size == 0)
        {
            /* no child nodes, lets clean up this node */
            smap__destroy(node, NULL);

            /* make sure to set the node to NULL so the parent
             * can do its cleanup correctly */
            *nd = NULL;

            /* size of parent should be minus one */
            smap__dec_node(parent);

            return data;
        }
        /* we cannot clean this node, set data to NULL */
        node->data = NULL;

        if (node->size == 1)
        {
            /* we have only one child, we can merge this
             * child with this one */
            smap__merge_node(node);
        }

        return data;
    }

    if (node->nodes)
    {
        uint8_t k = (uint8_t) *key;
        uint8_t pos = k / SMAP_BSZ;

        if (pos < node->offset || pos >= node->offset + node->sz)
            return NULL;

        smap_node_t ** next = &(*node->nodes)[k - node->offset * SMAP_BSZ];

        return (*next) ? smap__pop(node, next, key + 1) : NULL;
    }
    return NULL;
}

/*
 * Returns NULL and raises a SIGNAL in case an error has occurred.
 */
static smap_node_t * smap__node_create(
        const char * key,
        size_t n,
        void * data)
{
    smap_node_t * node = malloc(sizeof(smap_node_t));
    if (!node)
        return NULL;

    node->n = n;
    node->data = data;
    node->size = 0;
    node->offset = UINT8_MAX;
    node->sz = 0;
    node->nodes = NULL;

    if (n)
    {
        node->key = malloc(n);
        if (!node->key)
        {
            free(node);
            return NULL;
        }
        memcpy(node->key, key, n);
    }
    else
    {
        node->key = NULL;
    }
    return node;
}

/*
 * Returns 0 is successful or SMAP_ERR_ALLOC in case of an allocation error.
 *
 * (in case of an error, smap is remains unchanged)
 */
static int smap__node_resize(smap_node_t * node, uint8_t pos)
{
    int rc = 0;

    if (!node->nodes)
    {
        node->nodes = calloc(1, sizeof(smap_nodes_t));
        if (!node->nodes)
        {
            rc = SMAP_ERR_ALLOC;
        }
        else
        {
            node->offset = pos;
            node->sz = 1;
        }
    }
    else if (pos < node->offset)
    {
        smap_nodes_t * tmp;
        uint8_t diff = node->offset - pos;
        uint8_t oldn = node->sz;
        node->sz += diff;
        tmp = realloc(node->nodes, node->sz * sizeof(smap_nodes_t));
        if (!tmp && node->sz)
        {
            node->sz -= diff;
            rc = SMAP_ERR_ALLOC;
        }
        else
        {
            node->nodes = tmp;
            node->offset = pos;
            memmove(node->nodes + diff,
                    node->nodes,
                    oldn * sizeof(smap_nodes_t));
            memset(node->nodes, 0, diff * sizeof(smap_nodes_t));
        }
    }
    else if (pos >= node->offset + node->sz)
    {
        smap_nodes_t * tmp;
        uint8_t diff = pos - node->offset - node->sz + 1;
        uint8_t oldn = node->sz;
        node->sz += diff;  /* assert node->n > 0 */
        tmp = realloc(node->nodes, node->sz * sizeof(smap_nodes_t));
        if (!tmp)
        {
            node->sz -= diff;
            rc = SMAP_ERR_ALLOC;
        }
        else
        {
            node->nodes = tmp;
            memset(node->nodes + oldn, 0, diff * sizeof(smap_nodes_t));
        }
    }

    return rc;
}

/*
 * Destroy smap_tree. (parsing NULL is NOT allowed)
 * Call-back function will be called on each item in the tree.
 */
static void smap__destroy(smap_node_t * node, smap_destroy_cb cb)
{
    if (node->nodes)
    {
        for (uint_fast16_t i = 0, end = node->sz * SMAP_BSZ; i < end; i++)
        {
            if ((*node->nodes)[i])
            {
                smap__destroy((*node->nodes)[i], cb);
            }
        }
        free(node->nodes);
    }
    if (cb && node->data)
    {
        (*cb)(node->data);
    }
    free(node->key);
    free(node);
}

