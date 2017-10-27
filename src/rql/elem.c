/*
 * elem.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <rql/elem.h>
#include <rql/item.h>
#include <util/logger.h>

rql_elem_t * rql_elem_create(uint64_t id)
{
    rql_elem_t * elem = (rql_elem_t *) malloc(sizeof(rql_elem_t));
    if (!elem) return NULL;

    elem->ref = 1;
    elem->id = id;
    elem->items = vec_new(0);
    elem->flags = RQL_ELEM_FLAG_SWEEP;
    if (!elem->items)
    {
        rql_elem_drop(elem);
        return NULL;
    }
    return elem;
}


rql_elem_t * rql_elem_grab(rql_elem_t * elem)
{
    elem->ref++;
    return elem;
}

void rql_elem_drop(rql_elem_t * elem)
{
    if (elem && !--elem->ref)
    {
        vec_destroy(elem->items, (vec_destroy_cb) rql_item_destroy);
        free(elem);
    }
}

int rql_elem_set(rql_elem_t * elem, rql_prop_t * prop, rql_val_e tp, void * v)
{
    for (vec_each(elem->items, rql_item_t, item))
    {
        if (item->prop == prop)
        {
            rql_val_clear(&item->val);
            return rql_val_set(&item->val, tp, v);
        }
    }
    rql_item_t * item = rql_item_create(prop, tp, v);
    if (!item || vec_push(&elem->items, item))
    {
        rql_item_destroy(item);
        return -1;
    }

    return 0;
}

int rql_elem_weak_set(
        rql_elem_t * elem,
        rql_prop_t * prop,
        rql_val_e tp,
        void * v)
{
    for (vec_each(elem->items, rql_item_t, item))
    {
        if (item->prop == prop)
        {
            rql_val_clear(&item->val);
            rql_val_weak_set(&item->val, tp, v);
            return 0;
        }
    }
    rql_item_t * item = rql_item_weak_create(prop, tp, v);
    if (!item || vec_push(&elem->items, item))
    {
        rql_item_destroy(item);
        return -1;
    }
    return 0;
}

int rql_elem_to_packer(rql_elem_t * elem, qp_packer_t * packer)
{
    if (qp_add_map(&packer) ||
        qp_add_raw(packer, (const unsigned char *) RQL_API_ID, 2) ||
        qp_add_int64(packer, (int64_t) elem->id)) return -1;

    for (vec_each(elem->items, rql_item_t, item))
    {
        if (qp_add_raw_from_str(packer, item->prop->name) ||
            rql_val_to_packer(&item->val, packer)) return -1;
    }
    return qp_close_map(packer);
}
