#include <util/umap.h>
#include <tiinc.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <tiinc.h>

#define UMAP_NODE_SZ 16  /* 4-bit nibbles: 0..15 valid, 16 = fully array */

static ALWAYS_INLINE uint8_t umap__get_nibble(const uint8_t uuid[16],
                                              size_t pos)
{
    return (uuid[pos >> 1] >> ((~pos & 1) << 2)) & 0x0F;
}

static ALWAYS_INLINE void umap__set_nibble(uint8_t uuid[16],
                                           size_t pos,
                                           uint8_t nibble)
{
    size_t byte_idx = pos >> 1;
    if (pos & 1)
        uuid[byte_idx] = (uuid[byte_idx] & 0xF0) | (nibble & 0x0F);
    else
        uuid[byte_idx] = (uuid[byte_idx] & 0x0F) | ((nibble & 0x0F) << 4);
}

static ALWAYS_INLINE umap_node_t * umap__get_child(umap_node_t * node,
                                                   uint8_t nibble)
{
    if (node->key == UMAP_NODE_SZ)
        return node->nodes + nibble;
    return (node->key == nibble) ? node->nodes : NULL;
}

static inline int umap__node_grow(umap_node_t * node)
{
    umap_node_t * tmp = calloc(UMAP_NODE_SZ, sizeof(umap_node_t));
    if (!tmp)
        return -1;

    if (node->nodes)
    {
        memcpy(tmp + node->key, node->nodes, sizeof(umap_node_t));
        free(node->nodes);
    }
    node->nodes = tmp;
    node->key = UMAP_NODE_SZ;
    return 0;
}

umap_t * umap_create(void)
{
    return calloc(1, sizeof(umap_t));
}

static void umap__node_destroy(umap_node_t * node, umap_destroy_cb cb)
{
    if (node->suffix)
    {
        free(node->suffix);
        node->suffix = NULL;
    }

    if (node->data && cb)
    {
        cb(node->data);
        node->data = NULL;
    }

    if (node->nodes)
    {
        uint8_t count = (node->key == UMAP_NODE_SZ) ? UMAP_NODE_SZ : 1;
        for (uint8_t i = 0; i < count; i++)
        {
            umap__node_destroy(&node->nodes[i], cb);
        }
        free(node->nodes);
        node->nodes = NULL;
    }
}

void umap_destroy(umap_t * map, umap_destroy_cb cb)
{
    if (!map)
        return;
    umap__node_destroy(&map->root, cb);
    free(map);
}

void umap_clear(umap_t * map, umap_destroy_cb cb)
{
    if (!map)
        return;
    umap__node_destroy(&map->root, cb);
    memset(&map->root, 0, sizeof(umap_node_t));
    map->n = 0;
}

void * umap_get(umap_t * map, const uint8_t uuid[16])
{
    if (!map)
        return NULL;

    umap_node_t * nd = &map->root;
    size_t pos = 0;

    while (nd)
    {
        /* check suffix match if present */
        if (nd->suffix_len > 0)
        {
            for (size_t i = 0; i < nd->suffix_len; i++)
            {
                if (pos >= 32 || umap__get_nibble(uuid, pos) != nd->suffix[i])
                    return NULL;
                pos++;
            }
            return nd->data;
        }

        if (pos == 32)
            return nd->data;

        if (!nd->nodes)
            return NULL;

        uint8_t nibble = umap__get_nibble(uuid, pos);
        nd = umap__get_child(nd, nibble);
        pos++;
    }

    return NULL;
}

static void * umap__set(umap_node_t * node,
                        const uint8_t uuid[16],
                        size_t pos,
                        void * data)
{
    if (pos == 32)
    {
        void * ret = node->data ? node->data : data;
        node->data = data;
        return ret;
    }

    uint8_t nibble = umap__get_nibble(uuid, pos);
    umap_node_t * nd;

    if (!node->sz)
    {
        node->nodes = calloc(1, sizeof(umap_node_t));
        if (!node->nodes)
            return NULL;
        node->key = nibble;
        node->sz = 1;
        nd = node->nodes;
    }
    else if (node->key != nibble && node->key != UMAP_NODE_SZ)
    {
        if (umap__node_grow(node) != 0)
            return NULL;
        nd = node->nodes + nibble;
        node->sz++;
    }
    else
    {
        if (node->key != UMAP_NODE_SZ)
        {
            nd = node->nodes;
        }
        else
        {
            nd = node->nodes + nibble;
            if (!nd->nodes && !nd->data)
                node->sz++;
        }
    }

    return umap__set(nd, uuid, pos + 1, data);
}

void * umap_set(umap_t * map, const uint8_t uuid[16], void * data)
{
    assert(map != NULL);
    assert(data != NULL);

    void * ret = umap__set(&map->root, uuid, 0, data);
    if (ret == data)
        map->n++;

    return ret;
}

static void * umap__pop(umap_node_t * node, const uint8_t uuid[16], size_t pos)
{
    if (pos == 32)
    {
        void * data = node->data;
        node->data = NULL;
        return data;
    }

    if (!node->nodes)
        return NULL;

    uint8_t nibble = umap__get_nibble(uuid, pos);
    umap_node_t * child = umap__get_child(node, nibble);
    if (!child)
        return NULL;

    void * data = umap__pop(child, uuid, pos + 1);

    if (data && !child->data && !child->nodes)
    {
        if (node->key == UMAP_NODE_SZ)
        {
            node->sz--;
            if (node->sz == 0)
            {
                free(node->nodes);
                node->nodes = NULL;
            }
        }
        else
        {
            free(node->nodes);
            node->nodes = NULL;
            node->sz = 0;
        }
    }

    return data;
}

void * umap_pop(umap_t * map, const uint8_t uuid[16])
{
    if (map->n == 0)
        return NULL;

    void * data = umap__pop(&map->root, uuid, 0);
    if (data)
        map->n--;

    return data;
}

static int umap__walk(umap_node_t * node,
                      umap_cb cb,
                      void * arg,
                      uint8_t current_uuid[16],
                      size_t pos)
{
    int rc;

    // process suffix nibbles if present
    if (node->suffix_len > 0)
    {
        for (size_t i = 0; i < node->suffix_len; i++)
        {
            umap__set_nibble(current_uuid, pos + i, node->suffix[i]);
        }
        pos += node->suffix_len;
    }

    // invoke callback if value exists at this node
    if (node->data)
    {
        rc = cb(current_uuid, node->data, arg);
        if (rc != 0)
            return rc;
    }

    // recurse down active children
    if (node->nodes)
    {
        if (node->key == UMAP_NODE_SZ)
        {
            for (uint8_t nibble = 0; nibble < UMAP_NODE_SZ; nibble++)
            {
                umap_node_t * child = node->nodes + nibble;
                if (child->nodes || child->data || child->suffix_len)
                {
                    umap__set_nibble(current_uuid, pos, nibble);
                    rc = umap__walk(child, cb, arg, current_uuid, pos + 1);
                    if (rc != 0)
                        return rc;
                }
            }
        }
        else
        {
            umap__set_nibble(current_uuid, pos, node->key);
            rc = umap__walk(node->nodes, cb, arg, current_uuid, pos + 1);
            if (rc != 0)
                return rc;
        }
    }

    return 0;
}

/*
 * Run the call-back function on all items in the map.
 *
 * Walking stops on the first callback returning a non zero value.
 * The return value is the last callback result. A return value of 0 means that
 * the callback function is called on all items in the map.
 */
int umap_walk(umap_t * map, umap_cb cb, void * arg)
{
    if (map->n == 0)
        return 0;

    uint8_t current_uuid[16] = {0};
    return umap__walk(&map->root, cb, arg, current_uuid, 0);
}
