/*
 * imap.c
 *
 *  Created on: Sep 30, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <assert.h>
#include <stdlib.h>
#include <imap/imap.h>

#define IMAP_NODE_SZ 32

static void IMAP_node_free(imap_node_t * node);
static void IMAP_node_free_cb(imap_node_t * node, imap_free_cb cb);
static int IMAP_set(imap_node_t * node, uint64_t id, void * data);
static int IMAP_add(imap_node_t * node, uint64_t id, void * data);
static void * IMAP_get(imap_node_t * node, uint64_t id);
static void * IMAP_pop(imap_node_t * node, uint64_t id);
static void IMAP_walk(imap_node_t * node, imap_cb cb, void * data, int * rc);
static void IMAP_walkn(imap_node_t * node, imap_cb cb, void * data, size_t * n);
static void IMAP_2vec(imap_node_t * node, vec_t * vec);
static void IMAP_union_ref(imap_node_t * dest, imap_node_t * node);
static void IMAP_intersection_ref(
        imap_node_t * dest,
        imap_node_t * node,
        imap_free_cb decref_cb);
static void IMAP_difference_ref(
        imap_node_t * dest,
        imap_node_t * node,
        imap_free_cb decref_cb);
static void IMAP_symmetric_difference_ref(
        imap_node_t * dest,
        imap_node_t * node,
        imap_free_cb decref_cb);

/*
 * Returns a new imap or NULL in case of an allocation error.
 */
imap_t * imap_create(void)
{
    imap_t * imap = (imap_t *) calloc(
            1,
            sizeof(imap_t) + IMAP_NODE_SZ * sizeof(imap_node_t));
    if (!imap) return NULL;

    imap->n = 0;
    imap->vec = NULL;

    return imap;
}

/*
 * Destroy imap with optional call-back function.
 */
void imap_destroy(imap_t * imap, imap_free_cb cb)
{
    if (imap->n)
    {
        imap_node_t * nd;

        if (!cb)
        {
            for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
            {
                nd = imap->nodes + i;

                if (nd->nodes != NULL)
                {
                    IMAP_node_free(nd);
                }
            }
        }
        else
        {
            for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
            {
                nd = imap->nodes + i;

                if (nd->data != NULL)
                {
                    (*cb)(nd->data);
                }

                if (nd->nodes != NULL)
                {
                    IMAP_node_free_cb(nd, cb);
                }
            }
        }
    }

    vec_free(imap->vec);
    free(imap);
}



/*
 * Add data by id to the map.
 *
 * Returns 0 when data is overwritten and 1 if a new id/value is set. In case
 * of an allocation error the return value is IMAP_ERR_ALLOC.
 */
int imap_set(imap_t * imap, uint64_t id, void * data)
{
    assert (data != NULL);

    int rc;
    imap_node_t * nd = imap->nodes + (id % IMAP_NODE_SZ);
    id /= IMAP_NODE_SZ;

    if (!id)
    {
        rc = (nd->data == NULL);

        imap->n += rc;
        nd->data = data;
    }
    else
    {
        rc = IMAP_set(nd, id - 1, data);

        if (rc > 0)
        {
            imap->n++;
        }
    }

    if (imap->vec != NULL && (
            rc < 1 || vec_append_safe(&imap->vec, data)))
    {
        vec_free(imap->vec);
        imap->vec = NULL;
    }

    return rc;
}

/*
 * Add data by id to the map.
 *
 * Returns 0 when data is added. Data will NOT be overwritten.
 *
 * In case of a memory error the return value is IMAP_ERR_ALLOC. When the id
 * already exists the return value is IMAP_ERR_EXIST
 */
int imap_add(imap_t * imap, uint64_t id, void * data)
{
    assert (data != NULL);
    imap_node_t * nd = imap->nodes + (id % IMAP_NODE_SZ);
    id /= IMAP_NODE_SZ;

    if (!id)
    {
        if (nd->data != NULL)
        {
            return IMAP_ERR_EXIST;
        }

        imap->n++;
        nd->data = data;
    }
    else
    {
        int rc = IMAP_add(nd, id - 1, data);
        if (rc)
        {
            return rc;
        }
        imap->n++;
    }

    if (imap->vec != NULL && vec_append_safe(&imap->vec, data))
    {
        vec_free(imap->vec);
        imap->vec = NULL;
    }

    return 0;
}

/*
 * Returns data by a given id, or NULL when not found.
 */
void * imap_get(imap_t * imap, uint64_t id)
{
    imap_node_t * nd = imap->nodes + (id % IMAP_NODE_SZ);
    id /= IMAP_NODE_SZ;

    if (!id)
    {
        return nd->data;
    }

    return (nd->nodes) ? IMAP_get(nd, id - 1) : NULL;
}

/*
 * Remove and return an item by id or return NULL in case the id is not found.
 */
void * imap_pop(imap_t * imap, uint64_t id)
{
    void * data;
    imap_node_t * nd = imap->nodes + (id % IMAP_NODE_SZ);
    id /= IMAP_NODE_SZ;

    if (id)
    {
        data = (nd->nodes) ? IMAP_pop(nd, id - 1) : NULL;
    }
    else if ((data = nd->data) != NULL)
    {
        nd->data = NULL;
    }

    if (data != NULL)
    {
        imap->n--;

        if (imap->vec != NULL)
        {
            vec_free(imap->vec);
            imap->vec = NULL;
        }
    }

    return data;
}

/*
 * Run the call-back function on all items in the map.
 *
 * All the results are added together and are returned as the result of
 * this function.
 */
int imap_walk(imap_t * imap, imap_cb cb, void * data)
{
    int rc = 0;

    if (imap->n)
    {
        imap_node_t * nd;

        for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
        {
            nd = imap->nodes + i;

            if (nd->data != NULL)
            {
                rc += (*cb)(nd->data, data);
            }

            if (nd->nodes != NULL)
            {
                IMAP_walk(nd, cb, data, &rc);
            }
        }
    }

    return rc;
}

/*
 * Recursive function, call-back function will be called on each item.
 *
 * Walking stops either when the call-back is called on each value or
 * when 'n' is zero. 'n' will be decremented by the result of each call-back.
 */
void imap_walkn(imap_t * imap, size_t * n, imap_cb cb, void * data)
{
    if (imap->n)
    {
        imap_node_t * nd;

        for (uint_fast8_t i = 0; *n && i < IMAP_NODE_SZ; i++)
        {
            nd = imap->nodes + i;

            if (nd->data != NULL && !(*n -= (*cb)(nd->data, data)))
            {
                return;
            }

            if (nd->nodes != NULL)
            {
                IMAP_walkn(nd, cb, data, n);
            }
        }
    }
}

/*
 * Returns a pointer to imap->vec or NULL in case an allocation error has
 * occurred.
 *
 * The imap->vec will be created if it does not yet exist.
 */
vec_t * imap_vec(imap_t * imap)
{
    if (!imap->vec)
    {
        imap->vec = vec_new(imap->n);

        if (imap->vec != NULL && imap->n)
        {
            imap_node_t * nd;

            for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
            {
                nd = imap->nodes + i;

                if (nd->data != NULL)
                {
                    vec_append(imap->vec, nd->data);
                }

                if (nd->nodes != NULL)
                {
                    IMAP_2vec(nd, imap->vec);
                }
            }
        }
    }
    return imap->vec;
}

/*
 * Returns NULL and raises a SIGNAL in case an error has occurred.
 *
 * When successful a the vec is returned and imap->vec is set to NULL.
 *
 * This can be used when being sure this is the only time you need the list
 * and prevents making a copy.
 */
vec_t * imap_vec_pop(imap_t * imap)
{
    vec_t * vec = imap_vec(imap);
    imap->vec = NULL;
    return vec;
}

/*
 * Map 'dest' will be the union between the two maps. Map 'imap' will be
 * destroyed so it cannot be used anymore.
 *
 * This function can call 'decref_cb' when an item is removed from the map.
 * We only call the function for sure when the item is removed from both maps.
 * When we are sure the item still exists in the 'dest' map and is only removed
 * from the 'imap', we simply decrement the ref counter.
 */
void imap_union_ref(
        imap_t * dest,
        imap_t * imap,
        imap_free_cb decref_cb __attribute__((unused)))
{
    if (dest->vec != NULL)
    {
        vec_free(dest->vec);
        dest->vec = NULL;
    }

    if (imap->n)
    {
        imap_node_t * dest_nd;
        imap_node_t * imap_nd;

        for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
        {
            dest_nd = dest->nodes + i;
            imap_nd = imap->nodes + i;

            if (imap_nd->data != NULL)
            {
                if (dest_nd->data != NULL)
                {
                    /* we are sure there is a reference left */
                    rql_ref_dec((rql_ref_t *) imap_nd->data);
                }
                else
                {
                    dest_nd->data = imap_nd->data;
                    dest->n++;
                }
            }

            if (imap_nd->nodes != NULL)
            {
                if (dest_nd->nodes != NULL)
                {
                    size_t tmp = dest_nd->sz;
                    IMAP_union_ref(dest_nd, imap_nd);
                    dest->n += dest_nd->sz - tmp;
                }
                else
                {
                    dest_nd->nodes = imap_nd->nodes;
                    dest_nd->sz = imap_nd->sz;
                    dest->n += dest_nd->sz;
                }
            }
        }
    }

    /* cleanup source imap */
    vec_free(imap->vec);
    free(imap);
}

/*
 * Map 'dest' will be the intersection between the two maps. Map 'imap' will be
 * destroyed so it cannot be used anymore.
 *
 * This function can call 'decref_cb' when an item is removed from the map.
 * We only call the function for sure when the item is removed from both maps.
 * When we are sure the item still exists in the 'dest' map and is only removed
 * from the 'imap', we simply decrement the ref counter.
 */
void imap_intersection_ref(
        imap_t * dest,
        imap_t * imap,
        imap_free_cb decref_cb)
{
    if (dest->vec != NULL)
    {
        vec_free(dest->vec);
        dest->vec = NULL;
    }

    imap_node_t * dest_nd;
    imap_node_t * imap_nd;

    for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
    {
        dest_nd = dest->nodes + i;
        imap_nd = imap->nodes + i;
        if (imap_nd->data != NULL)
        {
            (*decref_cb)(imap_nd->data);
        }
        else if (dest_nd->data != NULL)
        {
            (*decref_cb)(dest_nd->data);
            dest_nd->data = NULL;
            dest->n--;
        }

        if (imap_nd->nodes != NULL)
        {
            if (dest_nd->nodes != NULL)
            {
                size_t tmp = dest_nd->sz;
                IMAP_intersection_ref(dest_nd, imap_nd, decref_cb);
                dest->n -= tmp - dest_nd->sz;
            }
            else
            {
                IMAP_node_free_cb(imap_nd, decref_cb);
            }
        }
        else if (dest_nd->nodes != NULL)
        {
            dest->n -= dest_nd->sz;
            IMAP_node_free_cb(dest_nd, decref_cb);
            dest_nd->nodes = NULL;
        }
    }

    /* cleanup source imap */
    vec_free(imap->vec);
    free(imap);
}

/*
 * Map 'dest' will be the difference between the two maps. Map 'imap' will be
 * destroyed so it cannot be used anymore.
 *
 * This function can call 'decref_cb' when an item is removed from the map.
 * We only call the function for sure when the item is removed from both maps.
 * When we are sure the item still exists in the 'dest' map and is only removed
 * from the 'imap', we simply decrement the ref counter.
 */
void imap_difference_ref(
        imap_t * dest,
        imap_t * imap,
        imap_free_cb decref_cb)
{
    if (dest->vec != NULL)
    {
        vec_free(dest->vec);
        dest->vec = NULL;
    }

    if (imap->n)
    {
        imap_node_t * dest_nd;
        imap_node_t * imap_nd;

        for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
        {
            dest_nd = dest->nodes + i;
            imap_nd = imap->nodes + i;

            if (imap_nd->data != NULL)
            {
                if (dest_nd->data != NULL)
                {
                    /* we are sure to have one reference left */
                    rql_ref_dec((rql_ref_t *) dest_nd->data);
                    dest_nd->data = NULL;
                    dest->n--;

                }
                /* now we are not sure anymore if we have reference left */
                (*decref_cb)(imap_nd->data);
            }

            if (imap_nd->nodes != NULL)
            {
                if (dest_nd->nodes != NULL)
                {
                    size_t tmp = dest_nd->sz;
                    IMAP_difference_ref(dest_nd, imap_nd, decref_cb);
                    dest->n -= tmp - dest_nd->sz;
                }
                else
                {
                    IMAP_node_free_cb(imap_nd, decref_cb);
                }
            }
        }
    }

    /* cleanup source imap */
    vec_free(imap->vec);
    free(imap);
}

/*
 * Map 'dest' will be the symmetric difference between the two maps. Map 'imap'
 * will be destroyed so it cannot be used anymore.
 *
 * This function can call 'decref_cb' when an item is removed from the map.
 * We only call the function for sure when the item is removed from both maps.
 * When we are sure the item still exists in the 'dest' map and is only removed
 * from the 'imap', we simply decrement the ref counter.
 */
void imap_symmetric_difference_ref(
        imap_t * dest,
        imap_t * imap,
        imap_free_cb decref_cb)
{
    if (dest->vec != NULL)
    {
        vec_free(dest->vec);
        dest->vec = NULL;
    }

    if (imap->n)
    {
        imap_node_t * dest_nd;
        imap_node_t * imap_nd;

        for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
        {
            dest_nd = dest->nodes + i;
            imap_nd = imap->nodes + i;

            if (imap_nd->data != NULL)
            {
                if (dest_nd->data != NULL)
                {
                    /* we are sure to have one reference left */
                    rql_ref_dec((rql_ref_t *) dest_nd->data);

                    /* but now we are not sure anymore */
                    (*decref_cb)(imap_nd->data);

                    dest_nd->data = NULL;
                    dest->n--;
                }
                else
                {
                    dest_nd->data = imap_nd->data;
                    dest->n++;
                }
            }

            if (imap_nd->nodes != NULL)
            {
                if (dest_nd->nodes != NULL)
                {
                    size_t tmp = dest_nd->sz;
                    IMAP_symmetric_difference_ref(
                            dest_nd,
                            imap_nd,
                            decref_cb);
                    dest->n += dest_nd->sz - tmp;
                }
                else
                {
                    dest_nd->nodes = imap_nd->nodes;
                    dest_nd->sz = imap_nd->sz;
                    dest->n += dest_nd->sz;
                }
            }
        }
    }

    /* cleanup source imap */
    vec_free(imap->vec);
    free(imap);
}

static void IMAP_node_free(imap_node_t * node)
{
    imap_node_t * nd;

    for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
    {
        if ((nd = node->nodes + i)->nodes != NULL)
        {
            IMAP_node_free(nd);
        }
    }

    free(node->nodes);
}

static void IMAP_node_free_cb(imap_node_t * node, imap_free_cb cb)
{
    imap_node_t * nd;

    for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
    {
        nd = node->nodes + i;

        if (nd->data != NULL)
        {
            (*cb)(nd->data);
        }

        if (nd->nodes != NULL)
        {
            IMAP_node_free_cb(nd, cb);
        }
    }
    free(node->nodes);
}

/*
 * Add data by id to the map.
 *
 * Returns 0 when data is overwritten and 1 if a new id/value is set.
 *
 * In case of an error we return IMAP_ERR_ALLOC and a SIGNAL is raised.
 */
static int IMAP_set(imap_node_t * node, uint64_t id, void * data)
{
    if (!node->sz)
    {
        node->nodes = (imap_node_t *) calloc(
                IMAP_NODE_SZ,
                sizeof(imap_node_t));

        if (!node->nodes) return IMAP_ERR_ALLOC;
    }

    int rc;
    imap_node_t * nd = node->nodes + (id % IMAP_NODE_SZ);
    id /= IMAP_NODE_SZ;

    if (!id)
    {
        rc = (nd->data == NULL);

        nd->data = data;
        node->sz += rc;

        return rc;
    }

    rc = IMAP_set(nd, id - 1, data);

    if (rc > 0)
    {
        node->sz++;
    }

    return rc;
}

/*
 * Add data by id to the map.
 *
 * Returns 0 when data is added. Data will NOT be overwritten.
 *
 * In case of a memory error we return IMAP_ERR_ALLOC and a SIGNAL is raised. If the id
 * already exists IMAP_ERR_EXIST will be returned.
 */
static int IMAP_add(imap_node_t * node, uint64_t id, void * data)
{
    if (!node->sz)
    {
        node->nodes = (imap_node_t *) calloc(
                IMAP_NODE_SZ,
                sizeof(imap_node_t));

        if (!node->nodes) return IMAP_ERR_ALLOC;
    }

    int rc;
    imap_node_t * nd = node->nodes + (id % IMAP_NODE_SZ);
    id /= IMAP_NODE_SZ;

    if (!id)
    {
        if (nd->data != NULL)
        {
            return IMAP_ERR_EXIST;
        }

        nd->data = data;
        node->sz++;

        return 0;
    }

    rc = IMAP_add(nd, id - 1, data);

    if (rc == 0)
    {
        node->sz++;
    }

    return rc;
}

static void * IMAP_get(imap_node_t * node, uint64_t id)
{
    imap_node_t * nd = node->nodes + (id % IMAP_NODE_SZ);
    id /= IMAP_NODE_SZ;

    if (!id)
    {
        return nd->data;
    }

    return (nd->nodes) ? IMAP_get(nd, id - 1) : NULL;
}

static void * IMAP_pop(imap_node_t * node, uint64_t id)
{
    void * data;
    imap_node_t * nd = node->nodes + (id % IMAP_NODE_SZ);
    id /= IMAP_NODE_SZ;

    if (!id)
    {
        if ((data = nd->data) != NULL)
        {
            if (--node->sz)
            {
                nd->data = NULL;
            }
            else
            {
                free(node->nodes);
                node->nodes = NULL;
            }
        }

        return data;
    }

    data = (nd->nodes) ? IMAP_pop(nd, id - 1) : NULL;

    if (data != NULL && !--node->sz)
    {
        free(node->nodes);
        node->nodes = NULL;
    }

    return data;
}

static void IMAP_walk(imap_node_t * node, imap_cb cb, void * data, int * rc)
{
    imap_node_t * nd;

    for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
    {
        nd = node->nodes + i;

        if (nd->data != NULL)
        {
            *rc += (*cb)(nd->data, data);
        }

        if (nd->nodes != NULL)
        {
            IMAP_walk(nd, cb, data, rc);
        }
    }
}

static void IMAP_walkn(imap_node_t * node, imap_cb cb, void * data, size_t * n)
{
    imap_node_t * nd;

    for (uint_fast8_t i = 0; *n && i < IMAP_NODE_SZ; i++)
    {
        nd = node->nodes + i;

        if (nd->data != NULL && !(*n -= (*cb)(nd->data, data)))
        {
             return;
        }

        if (nd->nodes != NULL)
        {
            IMAP_walkn(nd, cb, data, n);
        }
    }
}

static void IMAP_2vec(imap_node_t * node, vec_t * vec)
{
    imap_node_t * nd;

    for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
    {
        nd = node->nodes + i;

        if (nd->data != NULL)
        {
            vec_append(vec, nd->data);
        }

        if (nd->nodes != NULL)
        {
            IMAP_2vec(nd, vec);
        }
    }
}

static void IMAP_union_ref(imap_node_t * dest, imap_node_t * node)
{
    imap_node_t * dest_nd;
    imap_node_t * node_nd;

    for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
    {
        dest_nd = dest->nodes + i;
        node_nd = node->nodes + i;

        if (node_nd->data != NULL)
        {
            if (dest_nd->data != NULL)
            {
                /* we are sure there is a reference left */
                rql_ref_dec((rql_ref_t *) node_nd->data);
            }
            else
            {
                dest_nd->data = node_nd->data;
                dest->sz++;
            }
        }

        if (node_nd->nodes != NULL)
        {
            if (dest_nd->nodes != NULL)
            {
                size_t tmp = dest_nd->sz;
                IMAP_union_ref(dest_nd, node_nd);
                dest->sz += dest_nd->sz - tmp;
            }
            else
            {
                dest_nd->nodes = node_nd->nodes;
                dest_nd->sz = node_nd->sz;
                dest->sz += dest_nd->sz;
            }
        }
    }
    free(node->nodes);
}

static void IMAP_intersection_ref(
        imap_node_t * dest,
        imap_node_t * node,
        imap_free_cb decref_cb)
{
    imap_node_t * dest_nd;
    imap_node_t * node_nd;

    for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
    {
        dest_nd = dest->nodes + i;
        node_nd = node->nodes + i;

        if (node_nd->data != NULL)
        {
            (*decref_cb)(node_nd->data);
        }
        else if (dest_nd->data != NULL)
        {
            (*decref_cb)(dest_nd->data);
            dest_nd->data = NULL;
            dest->sz--;
        }

        if (node_nd->nodes != NULL)
        {
            if (dest_nd->nodes != NULL)
            {
                size_t tmp = dest_nd->sz;
                IMAP_intersection_ref(dest_nd, node_nd, decref_cb);
                dest->sz -= tmp - dest_nd->sz;
            }
            else
            {
                IMAP_node_free_cb(node_nd, decref_cb);
            }
        }
        else if (dest_nd->nodes != NULL)
        {
            dest->sz -= dest_nd->sz;
            IMAP_node_free_cb(dest_nd, decref_cb);
            dest_nd->nodes = NULL;
        }

    }

    if (!dest->sz)
    {
        IMAP_node_free(dest);
        dest->nodes = NULL;
    }

    free(node->nodes);
}

static void IMAP_difference_ref(
        imap_node_t * dest,
        imap_node_t * node,
        imap_free_cb decref_cb)
{
    imap_node_t * dest_nd;
    imap_node_t * node_nd;

    for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
    {
        dest_nd = dest->nodes + i;
        node_nd = node->nodes + i;

        if (node_nd->data != NULL)
        {
            if (dest_nd->data != NULL)
            {
                /* we are sure to have one ref left */
                rql_ref_dec((rql_ref_t *) dest_nd->data);
                dest_nd->data = NULL;
                dest->sz--;

            }
            /* now we are not sure anymore if we have ref left */
            (*decref_cb)(node_nd->data);
        }

        if (node_nd->nodes != NULL)
        {
            if (dest_nd->nodes != NULL)
            {
                size_t tmp = dest_nd->sz;
                IMAP_difference_ref(dest_nd, node_nd, decref_cb);
                dest->sz -= tmp - dest_nd->sz;
            }
            else
            {
                IMAP_node_free_cb(node_nd, decref_cb);
            }
        }
    }

    if (!dest->sz)
    {
        IMAP_node_free(dest);
        dest->nodes = NULL;
    }

    free(node->nodes);
}

static void IMAP_symmetric_difference_ref(
        imap_node_t * dest,
        imap_node_t * node,
        imap_free_cb decref_cb)
{
    imap_node_t * dest_nd;
    imap_node_t * node_nd;

    for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
    {
        dest_nd = dest->nodes + i;
        node_nd = node->nodes + i;

        if (node_nd->data != NULL)
        {
            if (dest_nd->data != NULL)
            {
                /* we are sure to have one ref left */
                rql_ref_dec((rql_ref_t *) dest_nd->data);

                /* but now we are not sure anymore */
                (*decref_cb)(node_nd->data);

                dest_nd->data = NULL;
                dest->sz--;
            }
            else
            {
                dest_nd->data = node_nd->data;
                dest->sz++;
            }
        }

        if (node_nd->nodes != NULL)
        {
            if (dest_nd->nodes != NULL)
            {
                size_t tmp = dest_nd->sz;
                IMAP_symmetric_difference_ref(
                        dest_nd,
                        node_nd,
                        decref_cb);
                dest->sz += dest_nd->sz - tmp;
            }
            else
            {
                dest_nd->nodes = node_nd->nodes;
                dest_nd->sz = node_nd->sz;
                dest->sz += dest_nd->sz;
            }
        }
    }

    if (!dest->sz)
    {
        IMAP_node_free(dest);
        dest->nodes = NULL;
    }

    free(node->nodes);
}
