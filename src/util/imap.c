/*
 * imap.c
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <util/imap.h>
#include <util/logger.h>
#include <tiinc.h>

#define IMAP_NODE_SZ 32

static void imap__node_destroy(imap_node_t * node);
static void imap__node_destroy_cb(imap_node_t * node, imap_destroy_cb cb);
static void * imap__set(imap_node_t * node, uint64_t id, void * data);
static int imap__add(imap_node_t * node, uint64_t id, void * data);
static void * imap__pop(imap_node_t * node, uint64_t id);
static int imap__walk(imap_node_t * node, imap_cb cb, void * arg);
static void imap__walkn(imap_node_t * node, imap_cb cb, void * arg, size_t * n);
static _Bool imap__eq(imap_node_t * nodea, imap_node_t * nodeb);
static void imap__vec(imap_node_t * node, vec_t * vec);
static uint64_t imap__unused_id(imap_node_t * node, uint64_t max);
static imap_node_t * imap__nodes_dup(imap_node_t * node);

/*
 * Returns a new imap or NULL in case of an allocation error.
 */
imap_t * imap_create(void)
{
    return calloc(1, sizeof(imap_t) + IMAP_NODE_SZ * sizeof(imap_node_t));
}

/*
 * Destroy imap with optional call-back function.
 */
void imap_destroy(imap_t * imap, imap_destroy_cb cb)
{
    if (!imap)
        return;

    if (imap->n)
    {
        imap_node_t * nd = imap->nodes, * end = nd + IMAP_NODE_SZ;
        if (!cb)
        {
            do
                if (nd->nodes)
                    imap__node_destroy(nd);
            while (++nd < end);
        }
        else
        {
            do
            {
                if (nd->data)
                    (*cb)(nd->data);

                if (nd->nodes)
                    imap__node_destroy_cb(nd, cb);
            }
            while (++nd < end);
        }
    }

    free(imap);
}

/*
 * Destroy imap with optional call-back function.
 */
void imap_clear(imap_t * imap, imap_destroy_cb cb)
{
    assert (imap);
    if (imap->n)
    {
        imap_node_t * nd = imap->nodes, * end = nd + IMAP_NODE_SZ;
        do
        {
            if (nd->data)
            {
                if (cb)
                    (*cb)(nd->data);
                nd->data = NULL;
            }

            if (nd->nodes)
            {
                imap__node_destroy_cb(nd, cb);
                nd->nodes = NULL;
            }
        }
        while (++nd < end);

        imap->n = 0;
    }
}


/*
 * Add data by id to the map. Data is not allowed to be NULL.
 *
 * Returns the data in case a new id is added to the map. Existing data will be
 * overwritten and if this happens the old data is returned. In case of an
 * allocation error the return value is NULL.
 */
void * imap_set(imap_t * imap, uint64_t id, void * data)
{
    assert (data != NULL);
    void * ret;
    imap_node_t * nd = imap->nodes + (id % IMAP_NODE_SZ);
    id /= IMAP_NODE_SZ;

    if (!id)
    {
        ret = nd->data ? nd->data : data;
        nd->data = data;
    }
    else
    {
        ret = imap__set(nd, id - 1, data);
    }

    imap->n += (ret == data);
    return ret;
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
        if (nd->data)
            return IMAP_ERR_EXIST;
        nd->data = data;
    }
    else
    {
        int rc = imap__add(nd, id - 1, data);
        if (rc)
            return rc;
    }
    imap->n++;
    return 0;
}

/*
 * Returns data by a given id, or NULL when not found.
 */
void * imap_get(imap_t * imap, uint64_t id)
{
    imap_node_t * nd = imap->nodes + (id % IMAP_NODE_SZ);
    do
    {
        id /= IMAP_NODE_SZ;

        if (!id)
            return nd->data;

        if (!nd->nodes)
            return NULL;

        nd = nd->nodes + (--id % IMAP_NODE_SZ);
    }
    while (1);
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
        data = nd->nodes ? imap__pop(nd, id - 1) : NULL;
    }
    else if ((data = nd->data))
    {
        nd->data = NULL;
    }

    if (data)
        imap->n--;

    return data;
}

/*
 * Run the call-back function on all items in the map.
 *
 * Walking stops on the first callback returning a non zero value.
 * The return value is the last callback result. A return value of 0 means that
 * the callback function is called on all items in the map.
 */
int imap_walk(imap_t * imap, imap_cb cb, void * arg)
{
    int rc = 0;

    if (imap->n)
    {
        imap_node_t * nd = imap->nodes, * end = nd + IMAP_NODE_SZ;
        do
        {
            if (nd->data && (rc = (*cb)(nd->data, arg)))
                return rc;

            if (nd->nodes && (rc = imap__walk(nd, cb, arg)))
                return rc;
        }
        while (++nd < end);
    }

    return rc;
}

/*
 * Recursive function, call-back function will be called on each item.
 *
 * Walking stops either when the call-back is called on each value or
 * when 'n' is zero. 'n' will be decremented by the result of each call-back.
 */
void imap_walkn(imap_t * imap, size_t * n, imap_cb cb, void * arg)
{
    if (imap->n)
    {
        imap_node_t * nd = imap->nodes, * end = nd + IMAP_NODE_SZ;
        do
        {
            if (nd->data && !(*n -= (*cb)(nd->data, arg)))
                return;

            if (nd->nodes)
                imap__walkn(nd, cb, arg, n);
        }
        while (++nd < end);
    }
}

/*
 * Returns `true` if the given imap objects are equal
 */
_Bool imap__eq_(imap_t * a, imap_t * b)
{
    imap_node_t
            * nda = a->nodes,
            * ndb = b->nodes,
            * end = nda + IMAP_NODE_SZ;

    assert (a != b && a->n == b->n && a->n);

    for (; nda < end; ++nda, ++ndb)
        if (nda->data != ndb->data ||
            !nda->nodes != !ndb->nodes ||
            (nda->nodes && !imap__eq(nda, ndb)))
            return false;

    return true;
}

/*
 * Returns a pointer to imap->vec or NULL in case an allocation error has
 * occurred.
 *
 * The imap->vec will be created if it does not yet exist.
 */
vec_t * imap_vec(imap_t * imap)
{
    vec_t * vec = vec_new(imap->n);
    if (vec && imap->n)
    {
        imap_node_t * nd = imap->nodes, * end = nd + IMAP_NODE_SZ;
        do
        {
            if (nd->data)
                VEC_push(vec, nd->data);

            if (nd->nodes)
                imap__vec(nd, vec);
        }
        while (++nd < end);
    }
    return vec;
}

/*
 * Returns a free id below the given `max` value. If no free id is found, then
 * the returned value will be equal to `max`. (This does not mean that `max`
 * is free to use)
 *
 * Note: the return value is not necessary the lowest free id, for example, 64
 * might be returned as a free id while 33 is also a free id.
 */
uint64_t imap_unused_id(imap_t * imap, uint64_t max)
{
    imap_node_t * nd = imap->nodes;
    size_t i, n, m, r;

    if (!imap->n)
        return 0;

    for (i = 0; i < IMAP_NODE_SZ; ++i, ++nd)
        if (!nd->data)
            return i > max ? max : i;

    n = IMAP_NODE_SZ + IMAP_NODE_SZ;
    n = n < max ? n : max;
    m = max / IMAP_NODE_SZ;
    nd = imap->nodes;

    for (i = IMAP_NODE_SZ; i < n; ++i, ++nd)
    {
        if (!nd->nodes)
            return i;

        if ((r = imap__unused_id(nd, m)) < m)
        {
            r *= IMAP_NODE_SZ;
            r += i;
            if (r < max)
                return r;
        }
    }

    return max;
}

static void imap__union_move(imap_node_t * dest, imap_node_t * node)
{
    imap_node_t * dest_nd;
    imap_node_t * node_nd;

    for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
    {
        dest_nd = dest->nodes + i;
        node_nd = node->nodes + i;

        if (node_nd->data)
        {
            if (dest_nd->data)
            {
                /* we are sure there is a reference left */
                ti_decref((ti_ref_t *) node_nd->data);
            }
            else
            {
                dest_nd->data = node_nd->data;
                dest->sz++;
            }
        }

        if (node_nd->nodes)
        {
            if (dest_nd->nodes)
            {
                size_t tmp = dest_nd->sz;
                imap__union_move(dest_nd, node_nd);
                dest->sz += dest_nd->sz - tmp;
            }
            else
            {
                dest_nd->nodes = node_nd->nodes;
                dest->sz += (dest_nd->sz = node_nd->sz);
            }
        }
    }
    free(node->nodes);
}

void imap_union_move(imap_t * dest, imap_t * imap)
{
    if (imap->n)
    {
        imap_node_t * dest_nd;
        imap_node_t * imap_nd;

        for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
        {
            dest_nd = dest->nodes + i;
            imap_nd = imap->nodes + i;

            if (imap_nd->data)
            {
                if (dest_nd->data)
                {
                    /* we are sure there is a reference left */
                    ti_decref((ti_ref_t *) imap_nd->data);
                }
                else
                {
                    dest_nd->data = imap_nd->data;
                    dest->n++;
                }
            }

            if (imap_nd->nodes)
            {
                if (dest_nd->nodes)
                {
                    size_t tmp = dest_nd->sz;
                    imap__union_move(dest_nd, imap_nd);
                    dest->n += dest_nd->sz - tmp;
                }
                else
                {
                    dest_nd->nodes = imap_nd->nodes;
                    dest->n += (dest_nd->sz = imap_nd->sz);
                }
            }
        }
    }

    /* everything is clear */
    memset(imap, 0, sizeof(imap_t) + IMAP_NODE_SZ * sizeof(imap_node_t));
}

static int imap__union_make(imap_node_t * dest, imap_node_t * node)
{

    for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
    {
        imap_node_t * dest_nd = dest->nodes + i;
        imap_node_t * node_nd = node->nodes + i;

        if (node_nd->data && !dest_nd->data)
        {
            dest_nd->data = node_nd->data;
            ti_incref((ti_ref_t *) dest_nd->data);
            dest->sz++;
        }

        if (node_nd->nodes)
        {
            if (dest_nd->nodes)
            {
                size_t tmp = dest_nd->sz;
                if (imap__union_make(dest_nd, node_nd))
                    return -1;
                dest->sz += dest_nd->sz - tmp;
            }
            else
            {
                if (!(dest_nd->nodes = imap__nodes_dup(node_nd->nodes)))
                    return -1;
                dest->sz += (dest_nd->sz = node_nd->sz);
            }
        }
    }
    return 0;
}

int imap_union_make(imap_t * dest, imap_t * a, imap_t * b)
{
    if (a->n || b->n)
    {
        imap_node_t * dest_nd;
        imap_node_t * a_nd;
        imap_node_t * b_nd;

        for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
        {
            dest_nd = dest->nodes + i;
            a_nd = a->nodes + i;
            b_nd = b->nodes + i;

            dest_nd->data = a_nd->data ? a_nd->data : b_nd->data;
            if (dest_nd->data)
            {
                ti_incref((ti_ref_t *) dest_nd->data);
                ++dest->n;
            }

            if (a_nd->nodes)
            {
                if (!(dest_nd->nodes = imap__nodes_dup(a_nd->nodes)))
                    return -1;
                dest->n += (dest_nd->sz = a_nd->sz);
            }

            if (b_nd->nodes)
            {
                if (dest_nd->nodes)
                {
                    size_t tmp = dest_nd->sz;
                    if (imap__union_make(dest_nd, b_nd))
                        return -1;
                    dest->n += dest_nd->sz - tmp;
                }
                else
                {
                    if (!(dest_nd->nodes = imap__nodes_dup(b_nd->nodes)))
                        return -1;
                    dest->n += (dest_nd->sz = b_nd->sz);

                }
            }
        }
    }
    return 0;
}

static void imap__difference_inplace(imap_node_t * dest, imap_node_t * node)
{
    imap_node_t * dest_nd;
    imap_node_t * node_nd;

    for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
    {
        dest_nd = dest->nodes + i;
        node_nd = node->nodes + i;

        if (node_nd->data && dest_nd->data)
        {
            /* we are sure to have one reference left */
            ti_decref((ti_ref_t *) dest_nd->data);
            dest_nd->data = NULL;
            dest->sz--;
        }

        if (node_nd->nodes && dest_nd->nodes)
        {
            size_t tmp = dest_nd->sz;
            imap__difference_inplace(dest_nd, node_nd);
            dest->sz -= tmp - dest_nd->sz;
            if (!dest_nd->sz)
            {
                free(dest_nd->nodes);
                dest_nd->nodes = NULL;
            }
        }
    }
}

void imap_difference_inplace(imap_t * dest, imap_t * imap)
{
    if (imap->n)
    {
        imap_node_t * dest_nd;
        imap_node_t * imap_nd;

        for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
        {
            dest_nd = dest->nodes + i;
            imap_nd = imap->nodes + i;

            if (imap_nd->data && dest_nd->data)
            {
                /* we are sure to have one reference left */
                ti_decref((ti_ref_t *) dest_nd->data);
                dest_nd->data = NULL;
                dest->n--;
            }

            if (imap_nd->nodes && dest_nd->nodes)
            {
                size_t tmp = dest_nd->sz;
                imap__difference_inplace(dest_nd, imap_nd);
                dest->n -= tmp - dest_nd->sz;
                if (!dest_nd->sz)
                {
                    free(dest_nd->nodes);
                    dest_nd->nodes = NULL;
                }
            }
        }
    }
}

static int imap__difference_make(
        imap_node_t * dest,
        imap_node_t * a,
        imap_node_t * b)
{
    dest->nodes = calloc(IMAP_NODE_SZ, sizeof(imap_node_t));
    if (!dest->nodes)
        return -1;

    for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
    {
        imap_node_t * dest_nd = dest->nodes + i;
        imap_node_t * a_nd = a->nodes + i;
        imap_node_t * b_nd = b->nodes + i;

        if (a_nd->data && !b_nd->data)
        {
            dest_nd->data = a_nd->data;
            ti_incref((ti_ref_t *) dest_nd->data);
            ++dest->sz;
        }

        if (a_nd->nodes)
        {
            if (!b_nd->nodes)
            {
                if (!(dest_nd->nodes = imap__nodes_dup(a_nd->nodes)))
                    return -1;
                dest->sz += (dest_nd->sz = a_nd->sz);
            }
            else if (a_nd->sz != b_nd->sz || !imap__eq(a_nd, b_nd))
            {
                if (imap__difference_make(dest_nd, a_nd, b_nd))
                    return -1;
                dest->sz += dest_nd->sz;
            }
        }
    }
    return 0;
}

int imap_difference_make(imap_t * dest, imap_t * a, imap_t * b)
{
    if (a->n)
    {
        imap_node_t * dest_nd;
        imap_node_t * a_nd;
        imap_node_t * b_nd;

        for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
        {
            dest_nd = dest->nodes + i;
            a_nd = a->nodes + i;
            b_nd = b->nodes + i;

            if (a_nd->data && !b_nd->data)
            {
                dest_nd->data = a_nd->data;
                ti_incref((ti_ref_t *) dest_nd->data);
                ++dest->n;
            }

            if (a_nd->nodes)
            {
                if (!b_nd->nodes)
                {
                    if (!(dest_nd->nodes = imap__nodes_dup(a_nd->nodes)))
                        return -1;
                    dest->n += (dest_nd->sz = a_nd->sz);
                }
                else if (a_nd->sz != b_nd->sz || !imap__eq(a_nd, b_nd))
                {
                    if (imap__difference_make(dest_nd, a_nd, b_nd))
                        return -1;
                    dest->n += dest_nd->sz;
                }
            }
        }
    }
    return 0;
}

static void imap__intersection_inplace(
        imap_node_t * dest,
        imap_node_t * node,
        imap_destroy_cb cb)
{
    imap_node_t * dest_nd;
    imap_node_t * node_nd;

    for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
    {
        dest_nd = dest->nodes + i;
        node_nd = node->nodes + i;

        if (!node_nd->data && dest_nd->data)
        {
            (*cb)(dest_nd->data);
            dest_nd->data = NULL;
            dest->sz--;
        }

        if (dest_nd->nodes)
        {
            if (node_nd->nodes)
            {
                size_t tmp = dest_nd->sz;
                imap__intersection_inplace(dest_nd, node_nd, cb);
                dest->sz -= tmp - dest_nd->sz;
            }
            else
            {
                dest->sz -= dest_nd->sz;
                imap__node_destroy_cb(dest_nd, cb);
                dest_nd->nodes = NULL;
            }
        }
    }

    if (!dest->sz)
    {
        free(dest->nodes);
        dest->nodes = NULL;
    }
}

void imap_intersection_inplace(imap_t * dest, imap_t * imap, imap_destroy_cb cb)
{
    imap_node_t * dest_nd;
    imap_node_t * imap_nd;

    for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
    {
        dest_nd = dest->nodes + i;
        imap_nd = imap->nodes + i;

        if (!imap_nd->data && dest_nd->data)
        {
            (*cb)(dest_nd->data);
            dest_nd->data = NULL;
            dest->n--;
        }

        if (dest_nd->nodes)
        {
            if (imap_nd->nodes)
            {
                size_t tmp = dest_nd->sz;
                imap__intersection_inplace(dest_nd, imap_nd, cb);
                dest->n -= tmp - dest_nd->sz;
            }
            else
            {
                dest->n -= dest_nd->sz;
                imap__node_destroy_cb(dest_nd, cb);
                dest_nd->nodes = NULL;
            }
        }
    }
}

static int imap__intersection_make(
        imap_node_t * dest,
        imap_node_t * a,
        imap_node_t * b)
{
    dest->nodes = calloc(IMAP_NODE_SZ, sizeof(imap_node_t));
    if (!dest->nodes)
        return -1;

    for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
    {
        imap_node_t * dest_nd = dest->nodes + i;
        imap_node_t * a_nd = a->nodes + i;
        imap_node_t * b_nd = b->nodes + i;

        if (a_nd->data && b_nd->data)
        {
            dest_nd->data = a_nd->data;
            ti_incref((ti_ref_t *) dest_nd->data);
            ++dest->sz;
        }

        if (a_nd->nodes && b_nd->nodes)
        {
            if (imap__intersection_make(dest_nd, a_nd, b_nd))
                return -1;
            dest->sz += dest_nd->sz;
        }
    }

    if (!dest->sz)
    {
        free(dest->nodes);
        dest->nodes = NULL;
    }
    return 0;
}

int imap_intersection_make(imap_t * dest, imap_t * a, imap_t * b)
{
    if (a->n && b->n)
    {
        imap_node_t * dest_nd;
        imap_node_t * a_nd;
        imap_node_t * b_nd;

        for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
        {
            dest_nd = dest->nodes + i;
            a_nd = a->nodes + i;
            b_nd = b->nodes + i;

            if (a_nd->data && b_nd->data)
            {
                dest_nd->data = a_nd->data;
                ti_incref((ti_ref_t *) dest_nd->data);
                ++dest->n;
            }

            if (a_nd->nodes && b_nd->nodes)
            {
                if (imap__intersection_make(dest_nd, a_nd, b_nd))
                    return -1;
                dest->n += dest_nd->sz;
            }
        }
    }
    return 0;
}

static void imap__symmdiff_move(
        imap_node_t * dest,
        imap_node_t * node,
        imap_destroy_cb cb)
{
    imap_node_t * dest_nd;
    imap_node_t * node_nd;

    for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
    {
        dest_nd = dest->nodes + i;
        node_nd = node->nodes + i;

        if (node_nd->data)
        {
            if (dest_nd->data)
            {
                /* we are sure to have one reference left */
                ti_decref((ti_ref_t *) dest_nd->data);

                /* but now we are not sure anymore */
                (*cb)(node_nd->data);

                dest_nd->data = NULL;
                dest->sz--;
            }
            else
            {
                dest_nd->data = node_nd->data;
                dest->sz++;
            }
        }

        if (node_nd->nodes)
        {
            if (dest_nd->nodes)
            {
                size_t tmp = dest_nd->sz;
                imap__symmdiff_move(dest_nd, node_nd, cb);
                dest->sz += dest_nd->sz - tmp;
            }
            else
            {
                dest_nd->nodes = node_nd->nodes;
                dest->sz += (dest_nd->sz = node_nd->sz);
            }
        }
    }

    if (!dest->sz)
    {
        free(dest->nodes);
        dest->nodes = NULL;
    }

    free(node->nodes);
}

void imap_symmdiff_move(imap_t * dest, imap_t * imap, imap_destroy_cb cb)
{
    if (imap->n)
    {
        imap_node_t * dest_nd;
        imap_node_t * imap_nd;

        for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
        {
            dest_nd = dest->nodes + i;
            imap_nd = imap->nodes + i;

            if (imap_nd->data)
            {
                if (dest_nd->data)
                {
                    /* we are sure to have one reference left */
                    ti_decref((ti_ref_t *) dest_nd->data);

                    /* but now we are not sure anymore */
                    (*cb)(imap_nd->data);

                    dest_nd->data = NULL;
                    dest->n--;
                }
                else
                {
                    dest_nd->data = imap_nd->data;
                    dest->n++;
                }
            }


            if (imap_nd->nodes)
            {
                if (dest_nd->nodes)
                {
                    size_t tmp = dest_nd->sz;
                    imap__symmdiff_move(dest_nd, imap_nd, cb);
                    dest->n += dest_nd->sz - tmp;
                }
                else
                {
                    dest_nd->nodes = imap_nd->nodes;
                    dest->n += (dest_nd->sz = imap_nd->sz);
                }
            }
        }
    }

    /* everything is clear */
    memset(imap, 0, sizeof(imap_t) + IMAP_NODE_SZ * sizeof(imap_node_t));
}

static int imap__symmdiff_make(
        imap_node_t * dest,
        imap_node_t * a,
        imap_node_t * b)
{
    dest->nodes = calloc(IMAP_NODE_SZ, sizeof(imap_node_t));
    if (!dest->nodes)
        return -1;

    for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
    {
        imap_node_t * dest_nd = dest->nodes + i;
        imap_node_t * a_nd = a->nodes + i;
        imap_node_t * b_nd = b->nodes + i;

        if (a_nd->data && !b_nd->data)
        {
            dest_nd->data = a_nd->data;
            ti_incref((ti_ref_t *) dest_nd->data);
            ++dest->sz;
        }
        else if (!a_nd->data && b_nd->data)
        {
            dest_nd->data = b_nd->data;
            ti_incref((ti_ref_t *) dest_nd->data);
            ++dest->sz;
        }

        if (a_nd->nodes &&
            b_nd->nodes &&
            (a_nd->sz != b_nd->sz || !imap__eq(a_nd, b_nd)))
        {
            if (imap__symmdiff_make(dest_nd, a_nd, b_nd))
                return -1;
            dest->sz += dest_nd->sz;
        }
        else if (a_nd->nodes && !b_nd->nodes)
        {
            if (!(dest_nd->nodes = imap__nodes_dup(a_nd->nodes)))
                return -1;
            dest->sz +=(dest_nd->sz = a_nd->sz);
        }
        else if (!a_nd->nodes && b_nd->nodes)
        {
            if (!(dest_nd->nodes = imap__nodes_dup(b_nd->nodes)))
                return -1;
            dest->sz += (dest_nd->sz = b_nd->sz);
        }
    }
    return 0;
}

int imap_symmdiff_make(imap_t * dest, imap_t * a, imap_t * b)
{
    if (a->n || b->n)
    {
        imap_node_t * dest_nd;
        imap_node_t * a_nd;
        imap_node_t * b_nd;

        for (uint_fast8_t i = 0; i < IMAP_NODE_SZ; i++)
        {
            dest_nd = dest->nodes + i;
            a_nd = a->nodes + i;
            b_nd = b->nodes + i;

            if (a_nd->data && !b_nd->data)
            {
                dest_nd->data = a_nd->data;
                ti_incref((ti_ref_t *) dest_nd->data);
                ++dest->n;
            }
            else if (!a_nd->data && b_nd->data)
            {
                dest_nd->data = b_nd->data;
                ti_incref((ti_ref_t *) dest_nd->data);
                ++dest->n;
            }

            if (a_nd->nodes &&
                b_nd->nodes &&
                (a_nd->sz != b_nd->sz || !imap__eq(a_nd, b_nd)))
            {
                if (imap__symmdiff_make(dest_nd, a_nd, b_nd))
                    return -1;
                dest->n += dest_nd->sz;
            }
            else if (a_nd->nodes && !b_nd->nodes)
            {
                if (!(dest_nd->nodes = imap__nodes_dup(a_nd->nodes)))
                    return -1;
                dest->n +=(dest_nd->sz = a_nd->sz);
            }
            else if (!a_nd->nodes && b_nd->nodes)
            {
                if (!(dest_nd->nodes = imap__nodes_dup(b_nd->nodes)))
                    return -1;
                dest->n += (dest_nd->sz = b_nd->sz);
            }
        }
    }
    return 0;
}

static void imap__node_destroy(imap_node_t * node)
{
    imap_node_t * nd = node->nodes, * end = nd + IMAP_NODE_SZ;

    do
        if (nd->nodes)
            imap__node_destroy(nd);
    while (++nd < end);

    free(node->nodes);
}

static void imap__node_destroy_cb(imap_node_t * node, imap_destroy_cb cb)
{
    imap_node_t * nd = node->nodes, * end = nd + IMAP_NODE_SZ;

    do
    {
        if (nd->data)
            (*cb)(nd->data);

        if (nd->nodes)
            imap__node_destroy_cb(nd, cb);
    }
    while (++nd < end);

    free(node->nodes);
}

static void * imap__set(imap_node_t * node, uint64_t id, void * data)
{
    if (!node->sz)
    {
        node->nodes = calloc(IMAP_NODE_SZ, sizeof(imap_node_t));
        if (!node->nodes)
            return NULL;
    }

    void * ret;
    imap_node_t * nd = node->nodes + (id % IMAP_NODE_SZ);
    id /= IMAP_NODE_SZ;

    if (!id)
    {
        ret = nd->data ? nd->data : data;
        nd->data = data;
    }
    else
    {
        ret = imap__set(nd, id - 1, data);
    }

    node->sz += (ret == data);

    return ret;
}

static int imap__add(imap_node_t * node, uint64_t id, void * data)
{
    if (!node->sz)
    {
        node->nodes = calloc(IMAP_NODE_SZ, sizeof(imap_node_t));
        if (!node->nodes)
            return IMAP_ERR_ALLOC;
    }

    int rc;
    imap_node_t * nd = node->nodes + (id % IMAP_NODE_SZ);
    id /= IMAP_NODE_SZ;

    if (!id)
    {
        if (nd->data)
            return IMAP_ERR_EXIST;

        nd->data = data;
        node->sz++;

        return 0;
    }

    rc = imap__add(nd, id - 1, data);

    if (rc == 0)
    {
        node->sz++;
    }

    return rc;
}

static void * imap__pop(imap_node_t * node, uint64_t id)
{
    void * data;
    imap_node_t * nd = node->nodes + (id % IMAP_NODE_SZ);
    id /= IMAP_NODE_SZ;

    if (!id)
    {
        if ((data = nd->data))
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

    data = nd->nodes ? imap__pop(nd, id - 1) : NULL;

    if (data && !--node->sz)
    {
        free(node->nodes);
        node->nodes = NULL;
    }

    return data;
}

static int imap__walk(imap_node_t * node, imap_cb cb, void * arg)
{
    int rc;
    imap_node_t * nd = node->nodes, * end = nd + IMAP_NODE_SZ;

    do
    {
        if (nd->data && (rc = (*cb)(nd->data, arg)))
            return rc;

        if (nd->nodes && (rc = imap__walk(nd, cb, arg)))
            return rc;
    }
    while (++nd < end);

    return 0;
}

static void imap__walkn(imap_node_t * node, imap_cb cb, void * arg, size_t * n)
{
    imap_node_t * nd = node->nodes, * end = nd + IMAP_NODE_SZ;

    for (; *n && nd < end; ++nd)
    {
        if (nd->data && !(*n -= (*cb)(nd->data, arg)))
            return;

        if (nd->nodes)
            imap__walkn(nd, cb, arg, n);
    }
}

static _Bool imap__eq(imap_node_t * nodea, imap_node_t * nodeb)
{
    imap_node_t
            * nda = nodea->nodes,
            * ndb = nodeb->nodes,
            * end = nda + IMAP_NODE_SZ;

    for (; nda < end; ++nda, ++ndb)
        if (nda->sz != ndb->sz ||
            nda->data != ndb->data ||
            !nda->nodes != !ndb->nodes ||
            (nda->nodes && !imap__eq(nda, ndb)))
            return false;

    return true;
}

static void imap__vec(imap_node_t * node, vec_t * vec)
{
    imap_node_t * nd = node->nodes, * end = nd + IMAP_NODE_SZ;

    do
    {
        if (nd->data)
            VEC_push(vec, nd->data);

        if (nd->nodes)
            imap__vec(nd, vec);
    }
    while (++nd < end);
}

static uint64_t imap__unused_id(imap_node_t * node, uint64_t max)
{
    imap_node_t * nd = node->nodes;
    size_t i, n, m, r;

    for (i = 0; i < IMAP_NODE_SZ; ++i, ++nd)
        if (!nd->data)
            return i;  /* we don't care if larger than max */

    n = IMAP_NODE_SZ + IMAP_NODE_SZ;
    n = n < max ? n : max;
    m = max / IMAP_NODE_SZ - 1;
    nd = node->nodes;

    for (i = IMAP_NODE_SZ; i < n; ++i, ++nd)
    {
        if (!nd->nodes)
            return i;

        if ((r = imap__unused_id(nd, m)) < m)
        {
            r *= IMAP_NODE_SZ;
            r += i;
            return r;
        }
    }
    return max;
}

static inline void imap__nodes_dec(void * data)
{
    ti_decref((ti_ref_t *) data);
}

static imap_node_t * imap__nodes_dup(imap_node_t * node)
{
    imap_node_t * dest = calloc(IMAP_NODE_SZ, sizeof(imap_node_t));
    imap_node_t * ret = dest;
    imap_node_t * end = node + IMAP_NODE_SZ;

    if (!ret)
        return NULL;

    for (; node < end; ++node, ++dest)
    {
        if (node->data)
        {
            dest->data = node->data;
            ti_incref((ti_ref_t *) dest->data);
        }

        if (node->nodes && !(dest->nodes = imap__nodes_dup(node->nodes)))
        {
            imap__node_destroy_cb(ret, imap__nodes_dec);
            return NULL;
        }

        dest->sz = node->sz;
    }

    return ret;
}
